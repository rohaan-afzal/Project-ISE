#include "OrderService.h"
#include "Input-Validator.h"
#include "PaymentProcessor.h"
#include "OrderApprovalService.h"
#include "OrderWorkflowValidator.h"
#include "Record-Format.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

OrderService::OrderService(ProductService& prodService,
    OrderRepository& orderRepo,
    OrderDetailRepository& detailRepo,
    BillService& billSvc)
    : productService(prodService),
    orderRepository(orderRepo),
    detailRepository(detailRepo),
    billService(billSvc),
    inventoryService(prodService),
    paidOrderEditActive(false),
    resumedHeldOrderActive(false)
{
    startNewOrder();
}

std::string OrderService::currentDateString()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d");
    return oss.str();
}

std::string OrderService::currentTimeString()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%H:%M:%S");
    return oss.str();
}

std::string OrderService::generateNextOrderID() const
{
    const std::vector<Order> existing = orderRepository.getAllOrders();
    int highest = 1000;
    for (const auto& ord : existing)
    {
        int num = 0;
        if (ord.orderID.size() > 4 && ord.orderID.substr(0, 4) == "ORD-" &&
            RecordFormat::toInt(ord.orderID.substr(4), num) && num > highest)
        {
            highest = num;
        }
    }
    return "ORD-" + std::to_string(highest + 1);
}

void OrderService::startNewOrder(const std::string& customerName, const std::string& customerPhone)
{
    currentOrder = Order();
    currentOrder.orderID = generateNextOrderID();
    currentOrder.date = currentDateString();
    currentOrder.time = currentTimeString();
    currentOrder.customerName = customerName.empty() ? "Walk-in Customer" : customerName;
    currentOrder.customerPhone = customerPhone.empty() ? "-" : customerPhone;
    currentOrder.status = "in-progress";
    currentOrder.payment = Payment(0.0, "cash", "-");
}

void OrderService::setCustomerDetails(const std::string& customerName, const std::string& customerPhone)
{
    currentOrder.customerName = customerName.empty() ? "Walk-in Customer" : customerName;
    currentOrder.customerPhone = customerPhone.empty() ? "-" : customerPhone;
}

void OrderService::addProductByBarcode(const std::string& barcode, int quantity)
{
    if (currentOrder.status != "in-progress" && !paidOrderEditActive)
        throw std::runtime_error("This order is not open for editing.");
    if (!InputValidator::isValidBarcode(barcode))
        throw std::invalid_argument("Invalid barcode format. Expected a 6-digit barcode.");

    if (quantity <= 0)
        throw std::invalid_argument("Quantity must be at least 1.");

    Product product;
    if (!productService.getProductByBarcode(barcode, product))
        throw std::runtime_error("Product not found in catalog for barcode: " + barcode);

    for (auto& item : currentOrder.items)
    {
        if (item.product.barcode == barcode)
        {
            const int desiredTotal = item.quantity + quantity;
            int available = product.quantity;
            if (paidOrderEditActive) {
                for (const auto& original : originalPaidOrder.items)
                    if (original.product.barcode == barcode) available += original.quantity;
            }
            if (!InputValidator::isValidQuantity(desiredTotal, available))
            {
                throw std::runtime_error("Insufficient stock for " + product.name +
                    ". Available: " + std::to_string(product.quantity));
            }
            item.product = product;
            item.quantity = desiredTotal;
            return;
        }
    }

    int available = product.quantity;
    if (paidOrderEditActive) {
        for (const auto& original : originalPaidOrder.items)
            if (original.product.barcode == barcode) available += original.quantity;
    }
    if (!InputValidator::isValidQuantity(quantity, available))
    {
        throw std::runtime_error("Insufficient stock for " + product.name +
            ". Available: " + std::to_string(product.quantity));
    }

    currentOrder.items.emplace_back(product, quantity);
}

void OrderService::updateItemQuantity(const std::string& barcode, int newQuantity)
{
    if (currentOrder.status != "in-progress" && !paidOrderEditActive)
        throw std::runtime_error("This order is not open for editing.");
    if (newQuantity <= 0)
    {
        removeItem(barcode);
        return;
    }

    for (auto& item : currentOrder.items)
    {
        if (item.product.barcode == barcode)
        {
            Product fresh;
            int available = productService.getProductByBarcode(barcode, fresh)
                ? fresh.quantity
                : item.product.quantity;
            if (paidOrderEditActive) {
                for (const auto& original : originalPaidOrder.items)
                    if (original.product.barcode == barcode) available += original.quantity;
            }
            if (!InputValidator::isValidQuantity(newQuantity, available))
            {
                throw std::runtime_error("Requested quantity (" + std::to_string(newQuantity) +
                    ") exceeds available stock (" + std::to_string(available) + ").");
            }
            item.quantity = newQuantity;
            return;
        }
    }
    throw std::runtime_error("Item not found in current order.");
}

void OrderService::removeItem(const std::string& barcode)
{
    if (currentOrder.status != "in-progress" && !paidOrderEditActive)
        throw std::runtime_error("This order is not open for editing.");
    for (auto it = currentOrder.items.begin(); it != currentOrder.items.end(); ++it)
    {
        if (it->product.barcode == barcode)
        {
            currentOrder.items.erase(it);
            return;
        }
    }
}

void OrderService::clearOrder()
{
    paidOrderEditActive = false;
    originalPaidOrder = Order();
    resumedHeldOrderActive = false;
    persistedHeldOrder = Order();
    startNewOrder(currentOrder.customerName, currentOrder.customerPhone);
}

void OrderService::holdCurrentOrder()
{
    if (paidOrderEditActive)
        throw std::runtime_error("Paid order edits must be saved through payment checkout.");
    if (currentOrder.items.empty())
        throw std::runtime_error("Cannot place an empty order on hold.");

    const Order workingOrder = currentOrder;
    currentOrder.status = "on-hold";
    currentOrder.date = currentDateString();
    currentOrder.time = currentTimeString();
    try {
        orderRepository.saveOrder(currentOrder);
        detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
    } catch (...) {
        if (resumedHeldOrderActive) {
            orderRepository.saveOrder(persistedHeldOrder);
            detailRepository.saveOrderDetails(persistedHeldOrder.orderID, persistedHeldOrder.items);
        } else {
            orderRepository.removeOrder(currentOrder.orderID);
            detailRepository.saveOrderDetails(currentOrder.orderID, {});
        }
        currentOrder = workingOrder;
        throw;
    }
    resumedHeldOrderActive = false;
    persistedHeldOrder = Order();
    startNewOrder();
}

std::vector<Order> OrderService::getHeldOrders() const
{
    std::vector<Order> held;
    for (const auto& ord : orderRepository.getAllOrders())
    {
        if (OrderWorkflowValidator::canEditOnHoldOrder(ord))
            held.push_back(ord);
    }
    return held;
}

void OrderService::resumeOrder(const std::string& orderID)
{
    if (!currentOrder.items.empty())
        throw std::runtime_error("Hold or clear the current order before resuming another order.");
    for (auto ord : orderRepository.getAllOrders())
    {
        if (ord.orderID == orderID && OrderWorkflowValidator::canEditOnHoldOrder(ord))
        {
            ord.items = detailRepository.getDetailsForOrder(orderID, productService);
            if (ord.items.empty())
                throw std::runtime_error("The held order has no valid product details to resume.");
            persistedHeldOrder = ord;
            ord.status = "in-progress";
            resumedHeldOrderActive = true;
            currentOrder = ord;
            return;
        }
    }
    throw std::runtime_error("Held order not found: " + orderID);
}

std::vector<Order> OrderService::getEditablePaidOrders() const
{
    std::vector<Order> editable;
    for (const auto& order : orderRepository.getAllOrders())
        if (OrderWorkflowValidator::canEditPaidOrderWithinSevenDays(order))
            editable.push_back(order);
    return editable;
}

void OrderService::beginPaidOrderEdit(const std::string& orderID, const std::string& managerName,
                                      bool managerApproved)
{
    if (!currentOrder.items.empty())
        throw std::runtime_error("Hold or clear the current order before editing a paid order.");
    for (Order order : orderRepository.getAllOrders()) {
        if (order.orderID != orderID) continue;
        const std::string approver = OrderApprovalService::requireManagerApproval(
            order, managerApproved, managerName);
        order.items = detailRepository.getDetailsForOrder(orderID, productService);
        if (order.items.empty())
            throw std::runtime_error("The paid order has no valid product details to edit.");
        originalPaidOrder = order;
        order.managerApprovalName = approver;
        paidOrderEditActive = true;
        currentOrder = order;
        return;
    }
    throw std::runtime_error("Order not found: " + orderID);
}

void OrderService::archiveOrder(const std::string& orderID)
{
    for (Order order : orderRepository.getAllOrders()) {
        if (order.orderID != orderID) continue;
        if (order.status != "paid")
            throw std::runtime_error("Only paid orders can be archived.");
        order.status = "archived";
        orderRepository.saveOrder(order);
        return;
    }
    throw std::runtime_error("Order not found: " + orderID);
}

Bill OrderService::checkoutAndGenerateBill(const std::string& paymentMode,
    double amountTendered,
    const std::string& merchant)
{
    if (currentOrder.items.empty())
        throw std::runtime_error("Cannot checkout an empty order.");

    const double payable = getTotalPayable();
    const std::vector<BillLine> billLines = currentOrder.toBillLines();
    Bill bill = billService.createBill(currentOrder.customerName,
        currentOrder.orderID, "paid", billLines);

    if (paidOrderEditActive) {
        const Order previousOrder = originalPaidOrder;
        inventoryService.applyOrderChange(previousOrder.items, currentOrder.items);
        try {
            paymentService.updatePaidOrderPayment(currentOrder, previousOrder.payment,
                payable, amountTendered, paymentMode, merchant);
            // Preserve the original payment date so repeated edits cannot extend the
            // seven-day approval window.
            orderRepository.saveOrder(currentOrder);
            detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
            billService.saveBill(bill);
        } catch (...) {
            inventoryService.applyOrderChange(currentOrder.items, previousOrder.items);
            orderRepository.saveOrder(previousOrder);
            detailRepository.saveOrderDetails(previousOrder.orderID, previousOrder.items);
            throw;
        }
        paidOrderEditActive = false;
        originalPaidOrder = Order();
    } else {
        const Order orderBeforeCheckout = currentOrder;
        const bool wasResumedHeldOrder = resumedHeldOrderActive;
        const Order heldOrderBeforeResume = persistedHeldOrder;
        inventoryService.applyOrderChange({}, currentOrder.items);
        try {
            currentOrder.date = currentDateString();
            currentOrder.time = currentTimeString();
            paymentService.processPayment(currentOrder, payable, amountTendered, paymentMode, merchant);
            orderRepository.saveOrder(currentOrder);
            detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
            billService.saveBill(bill);
        } catch (...) {
            inventoryService.applyOrderChange(currentOrder.items, {});
            if (wasResumedHeldOrder) {
                orderRepository.saveOrder(heldOrderBeforeResume);
                detailRepository.saveOrderDetails(heldOrderBeforeResume.orderID, heldOrderBeforeResume.items);
            } else {
                orderRepository.removeOrder(currentOrder.orderID);
                detailRepository.saveOrderDetails(currentOrder.orderID, {});
            }
            currentOrder = orderBeforeCheckout;
            throw;
        }
        resumedHeldOrderActive = false;
        persistedHeldOrder = Order();
    }
    startNewOrder();
    return bill;
}

double OrderService::getSubtotal() const
{
    return PricingService::getTotalWithoutTax(currentOrder.toBillLines());
}

double OrderService::getTotalTax() const
{
    return PricingService::getTotalTax(currentOrder.toBillLines());
}

double OrderService::getTotalDiscount() const
{
    return PricingService::getTotalDiscount(currentOrder.toBillLines());
}

double OrderService::getTotalPayable() const
{
    return PricingService::getTotalPayable(currentOrder.toBillLines());
}
