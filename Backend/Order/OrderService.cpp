#include "OrderService.h"
#include "Input-Validator.h"
#include "PaymentProcessor.h"
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
    billService(billSvc)
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
            if (!InputValidator::isValidQuantity(desiredTotal, product.quantity))
            {
                throw std::runtime_error("Insufficient stock for " + product.name +
                    ". Available: " + std::to_string(product.quantity));
            }
            item.product = product;
            item.quantity = desiredTotal;
            return;
        }
    }

    if (!InputValidator::isValidQuantity(quantity, product.quantity))
    {
        throw std::runtime_error("Insufficient stock for " + product.name +
            ". Available: " + std::to_string(product.quantity));
    }

    currentOrder.items.emplace_back(product, quantity);
}

void OrderService::updateItemQuantity(const std::string& barcode, int newQuantity)
{
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
            const int available = productService.getProductByBarcode(barcode, fresh)
                ? fresh.quantity
                : item.product.quantity;
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
    startNewOrder(currentOrder.customerName, currentOrder.customerPhone);
}

void OrderService::holdCurrentOrder()
{
    if (currentOrder.items.empty())
        throw std::runtime_error("Cannot place an empty order on hold.");

    currentOrder.status = "on-hold";
    currentOrder.date = currentDateString();
    currentOrder.time = currentTimeString();
    orderRepository.saveOrder(currentOrder);
    detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
    startNewOrder();
}

std::vector<Order> OrderService::getHeldOrders() const
{
    std::vector<Order> held;
    for (const auto& ord : orderRepository.getAllOrders())
    {
        if (ord.status == "on-hold")
            held.push_back(ord);
    }
    return held;
}

void OrderService::resumeOrder(const std::string& orderID)
{
    for (auto ord : orderRepository.getAllOrders())
    {
        if (ord.orderID == orderID && ord.status == "on-hold")
        {
            ord.items = detailRepository.getDetailsForOrder(orderID, productService);
            ord.status = "in-progress";
            currentOrder = ord;
            return;
        }
    }
    throw std::runtime_error("Held order not found: " + orderID);
}

Bill OrderService::checkoutAndGenerateBill(const std::string& paymentMode,
    double amountTendered,
    const std::string& merchant)
{
    if (currentOrder.items.empty())
        throw std::runtime_error("Cannot checkout an empty order.");

    const double payable = getTotalPayable();
    const auto processor = PaymentProcessor::create(paymentMode);
    const Payment completedPayment = processor->process(payable, amountTendered, merchant);

    currentOrder.date = currentDateString();
    currentOrder.time = currentTimeString();
    currentOrder.status = "paid";
    currentOrder.payment = completedPayment;

    const std::vector<BillLine> billLines = currentOrder.toBillLines();
    Bill bill = billService.createBill(currentOrder.customerName,
        currentOrder.orderID,
        currentOrder.status,
        billLines);

    orderRepository.saveOrder(currentOrder);
    detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
    billService.saveBill(bill);

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