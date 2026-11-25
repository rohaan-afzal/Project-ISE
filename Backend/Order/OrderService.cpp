#include "OrderService.h"
#include "Input-Validator.h"
#include "PaymentProcessor.h"
#include "RecordFormat3.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <fstream>
#include <map>


OrderService::OrderService(ProductService& prodService,
                           OrderRepository& orderRepo,
                           OrderDetailRepository& detailRepo,
                           BillService& billSvc,
                           const SeasonalDiscountService* seasonSvc)
    : productService(prodService),
    orderRepository(orderRepo),
    detailRepository(detailRepo),
    billService(billSvc),
    seasonService(seasonSvc)
{
    startNewOrder();
}

std::string OrderService::currentDateString()
{
    return BillService::getCurrentDate();
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
                                           const std::string& merchant,
                                           double specialDiscountPercent)
{
    if (currentOrder.items.empty())
        throw std::runtime_error("Cannot checkout an empty order.");

    if (specialDiscountPercent < 0.0 || specialDiscountPercent > 8.0)
        throw std::invalid_argument("Special discount must be between 0% and 8%.");

    const double payable = getTotalPayable(specialDiscountPercent);
    const auto processor = PaymentProcessor::create(paymentMode);
    const Payment completedPayment = processor->process(payable, amountTendered, merchant);

    currentOrder.date = currentDateString();
    currentOrder.time = currentTimeString();
    currentOrder.status = "paid";
    currentOrder.payment = completedPayment;

    BillRequest request;
    request.customerName = currentOrder.customerName;
    request.orderID = currentOrder.orderID;
    request.paymentStatus = currentOrder.status;
    request.lines = currentOrder.toBillLines();
    request.amountReceived = (paymentMode == "cash") ? amountTendered : payable;
    request.specialDiscountPercent = specialDiscountPercent;
    request.billDate = currentOrder.date;
    request.billTime = currentOrder.time;

    Bill bill = billService.createBill(request);

    orderRepository.saveOrder(currentOrder);
    detailRepository.saveOrderDetails(currentOrder.orderID, currentOrder.items);
    billService.saveBill(bill);
    for (const auto& item : currentOrder.items)
    {
        if (!productService.deductStock(item.product.barcode, item.quantity))
        {
            throw std::runtime_error("Bill saved, but failed to update stock for barcode: " +
                                     item.product.barcode);
        }
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

double OrderService::getActiveSeasonalPercent() const
{
    if (!seasonService)
        return 0.0;
    return seasonService->getDiscountPercent(currentDateString());
}

double OrderService::getTotalPayable(double specialDiscountPercent) const
{
    const std::vector<BillLine> lines = currentOrder.toBillLines();
    const double afterProductDiscount = PricingService::getAmountAfterProductDiscount(lines);
    const double seasonalAmt = PricingService::getPercentAmount(afterProductDiscount, getActiveSeasonalPercent());
    const double specialAmt = PricingService::getPercentAmount(afterProductDiscount, specialDiscountPercent);
    return PricingService::getTotalPayable(afterProductDiscount, seasonalAmt, specialAmt);
}

namespace {
bool parseDateDMY(const std::string& dateStr, std::tm& outTm)
{
    if (dateStr.size() != 10 || dateStr[2] != '-' || dateStr[5] != '-')
        return false;
    int d = 0, m = 0, y = 0;
    if (!RecordFormat::toInt(dateStr.substr(0, 2), d) ||
        !RecordFormat::toInt(dateStr.substr(3, 2), m) ||
        !RecordFormat::toInt(dateStr.substr(6, 4), y))
        return false;

    outTm = std::tm{};
    outTm.tm_mday = d;
    outTm.tm_mon = m - 1;
    outTm.tm_year = y - 1900;
    outTm.tm_hour = 12;
    return true;
}
}

bool OrderService::verifyManagerPin(const std::string& pin) const
{
    std::string expectedPin = "1234";
    std::ifstream pinFile("Data/ManagerPin.txt");
    if (pinFile.is_open())
    {
        std::string filePin;
        if (std::getline(pinFile, filePin) && !filePin.empty())
            expectedPin = filePin;
    }
    return !pin.empty() && pin == expectedPin;
}

bool OrderService::isOrderEditableWithin7Days(const Order& order, std::string& reasonOut) const
{
    if (order.status != "paid")
    {
        reasonOut = "Only paid orders can be edited here (current status: " + order.status + ").";
        return false;
    }

    std::tm orderTm{};
    std::tm nowTm{};
    if (!parseDateDMY(order.date, orderTm) || !parseDateDMY(currentDateString(), nowTm))
    {
        reasonOut = "Invalid order date format (expected DD-MM-YYYY).";
        return false;
    }

    const std::time_t orderTime = std::mktime(&orderTm);
    const std::time_t nowTime = std::mktime(&nowTm);
    const double diffDays = std::difftime(nowTime, orderTime) / (60.0 * 60.0 * 24.0);

    if (diffDays < 0.0 || diffDays > 7.0)
    {
        reasonOut = "Order is outside the 7-day edit window (order date: " + order.date + ").";
        return false;
    }

    reasonOut.clear();
    return true;
}

std::vector<Order> OrderService::getEditablePaidOrders() const
{
    std::vector<Order> result;
    std::string reason;
    for (const auto& ord : orderRepository.getAllOrders())
    {
        if (isOrderEditableWithin7Days(ord, reason))
            result.push_back(ord);
    }
    return result;
}

Order OrderService::getOrderWithDetails(const std::string& orderID) const
{
    for (auto ord : orderRepository.getAllOrders())
    {
        if (ord.orderID == orderID)
        {
            ord.items = detailRepository.getDetailsForOrder(orderID, productService);
            return ord;
        }
    }
    throw std::runtime_error("Order not found: " + orderID);
}

Bill OrderService::saveEditedPaidOrder(const std::string& orderID,
                                       const std::vector<OrderItem>& updatedItems,
                                       const std::string& managerPin,
                                       double specialDiscountPercent)
{
    if (!verifyManagerPin(managerPin))
        throw std::runtime_error("Invalid Manager PIN. Authorization denied.");

    if (updatedItems.empty())
        throw std::invalid_argument("Edited order must contain at least one item.");

    if (specialDiscountPercent < 0.0 || specialDiscountPercent > 8.0)
        throw std::invalid_argument("Special discount must be between 0% and 8%.");

    Order target = getOrderWithDetails(orderID);
    std::string reason;
    if (!isOrderEditableWithin7Days(target, reason))
        throw std::runtime_error(reason);

    // Map old vs new quantities by barcode to validate and adjust inventory
    std::map<std::string, int> oldQtyMap;
    for (const auto& item : target.items)
        oldQtyMap[item.product.barcode] += item.quantity;

    std::map<std::string, int> newQtyMap;
    for (const auto& item : updatedItems)
    {
        if (item.quantity <= 0)
            throw std::invalid_argument("Item quantity must be at least 1.");
        newQtyMap[item.product.barcode] += item.quantity;
    }

    // 1. Validate sufficient stock for any net increases
    for (const auto& kv : newQtyMap)
    {
        const std::string& barcode = kv.first;
        const int delta = kv.second - oldQtyMap[barcode];
        if (delta > 0)
        {
            Product prod;
            if (!productService.getProductByBarcode(barcode, prod))
                throw std::runtime_error("Product not found for barcode: " + barcode);
            if (prod.quantity < delta)
                throw std::runtime_error("Insufficient stock for " + prod.name +
                                         " (need " + std::to_string(delta) + " more, available: " + std::to_string(prod.quantity) + ").");
        }
    }

    // 2. Apply net inventory changes
    for (const auto& kv : oldQtyMap)
    {
        const std::string& barcode = kv.first;
        const int delta = newQtyMap[barcode] - kv.second;
        if (delta < 0)
            productService.restockProduct(barcode, -delta);
        else if (delta > 0)
            productService.deductStock(barcode, delta);
    }
    for (const auto& kv : newQtyMap)
    {
        if (oldQtyMap.find(kv.first) == oldQtyMap.end() && kv.second > 0)
            productService.deductStock(kv.first, kv.second);
    }

    // 3. Recalculate bill & persist updated order + bill
    target.items = updatedItems;
    const std::vector<BillLine> lines = target.toBillLines();
    const double afterProdDisc = PricingService::getAmountAfterProductDiscount(lines);
    const double seasonalPct = seasonService ? seasonService->getDiscountPercent(target.date) : 0.0;
    const double seasonalAmt = PricingService::getPercentAmount(afterProdDisc, seasonalPct);
    const double specialAmt = PricingService::getPercentAmount(afterProdDisc, specialDiscountPercent);
    const double newPayable = PricingService::getTotalPayable(afterProdDisc, seasonalAmt, specialAmt);

    target.payment.amount = newPayable;
    orderRepository.saveOrder(target);
    detailRepository.replaceOrderDetails(target.orderID, target.items);

    BillRequest req;
    req.customerName = target.customerName;
    req.orderID = target.orderID;
    req.paymentStatus = "paid";
    req.lines = lines;
    req.amountReceived = newPayable;
    req.specialDiscountPercent = specialDiscountPercent;
    req.billDate = currentDateString();
    req.billTime = currentTimeString();

    Bill revisedBill = billService.createBill(req);
    billService.saveBill(revisedBill);
    return revisedBill;
}