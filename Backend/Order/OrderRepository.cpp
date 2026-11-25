#include "OrderRepository.h"
#include "Record-Format.h"
#include <fstream>
#include <map>
#include <stdexcept>

OrderRepository::OrderRepository(const std::string& path)
    : filePath(path)
{
}

std::vector<Order> OrderRepository::getAllOrders() const
{
    std::vector<Order> orders;
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return orders;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::vector<std::string> fields;
        if (!RecordFormat::splitLine(line, fields) || (fields.size() != 9 && fields.size() != 10))
            continue;

        Order order;
        order.orderID = RecordFormat::unescapeText(fields[0]);
        order.date = RecordFormat::unescapeText(fields[1]);
        order.time = RecordFormat::unescapeText(fields[2]);
        order.customerName = RecordFormat::unescapeText(fields[3]);
        order.customerPhone = RecordFormat::unescapeText(fields[4]);
        order.status = RecordFormat::unescapeText(fields[5]);
        order.payment.mode = RecordFormat::unescapeText(fields[6]);
        RecordFormat::toDouble(fields[7], order.payment.amount);
        order.payment.merchant = RecordFormat::unescapeText(fields[8]);
        if (fields.size() == 10)
            order.managerApprovalName = RecordFormat::unescapeText(fields[9]);

        orders.push_back(order);
    }

    return orders;
}

void OrderRepository::saveOrder(const Order& order) const
{
    std::vector<Order> all = getAllOrders();
    bool updated = false;
    for (auto& existing : all)
    {
        if (existing.orderID == order.orderID)
        {
            existing = order;
            updated = true;
            break;
        }
    }
    if (!updated)
        all.push_back(order);

    saveAllOrders(all);
}

void OrderRepository::removeOrder(const std::string& orderID) const
{
    std::vector<Order> remaining;
    for (const auto& order : getAllOrders())
        if (order.orderID != orderID) remaining.push_back(order);
    saveAllOrders(remaining);
}

void OrderRepository::saveAllOrders(const std::vector<Order>& orders) const
{
    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Could not open order file for writing: " + filePath);

    for (const auto& order : orders)
    {
        std::vector<std::string> fields = {
            RecordFormat::escapeText(order.orderID),
            RecordFormat::escapeText(order.date),
            RecordFormat::escapeText(order.time),
            RecordFormat::escapeText(order.customerName),
            RecordFormat::escapeText(order.customerPhone),
            RecordFormat::escapeText(order.status),
            RecordFormat::escapeText(order.payment.mode),
            RecordFormat::formatNumber(order.payment.amount),
            RecordFormat::escapeText(order.payment.merchant),
            RecordFormat::escapeText(order.managerApprovalName)
        };
        file << RecordFormat::joinFields(fields) << "\n";
    }

    if (!file.good())
        throw std::runtime_error("Failed writing order records to: " + filePath);
}

OrderDetailRepository::OrderDetailRepository(const std::string& path)
    : filePath(path)
{
}

void OrderDetailRepository::saveOrderDetails(const std::string& orderID, const std::vector<OrderItem>& items) const
{
    std::vector<std::string> retainedLines;
    {
        std::ifstream input(filePath.c_str());
        std::string line;
        while (std::getline(input, line)) {
            std::vector<std::string> fields;
            if (RecordFormat::splitLine(line, fields) && !fields.empty() &&
                RecordFormat::unescapeText(fields[0]) == orderID)
                continue;
            retainedLines.push_back(line);
        }
    }

    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Could not open order detail file for writing: " + filePath);
    for (const auto& line : retainedLines) file << line << "\n";
    for (const auto& item : items)
    {
        std::vector<std::string> fields = {
            RecordFormat::escapeText(orderID),
            RecordFormat::escapeText(item.product.productID),
            RecordFormat::escapeText(item.product.barcode),
            RecordFormat::escapeText(item.product.acronym),
            std::to_string(item.quantity)
        };
        file << RecordFormat::joinFields(fields) << "\n";
    }

    if (!file.good())
        throw std::runtime_error("Failed writing order details to: " + filePath);
}

std::vector<OrderItem> OrderDetailRepository::getDetailsForOrder(const std::string& orderID, const ProductService& productService) const
{
    std::vector<OrderItem> items;
    std::map<std::string, std::size_t> indexByBarcode;
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return items;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::vector<std::string> fields;
        if (!RecordFormat::splitLine(line, fields) || fields.size() != 5)
            continue;

        if (RecordFormat::unescapeText(fields[0]) != orderID)
            continue;

        const std::string barcode = RecordFormat::unescapeText(fields[2]);
        int qty = 0;
        if (!RecordFormat::toInt(fields[4], qty) || qty <= 0)
            continue;

        Product prod;
        if (productService.getProductByBarcode(barcode, prod))
        {
            const auto existing = indexByBarcode.find(barcode);
            if (existing == indexByBarcode.end()) {
                indexByBarcode.emplace(barcode, items.size());
                items.emplace_back(prod, qty);
            } else {
                items[existing->second].quantity += qty;
                items[existing->second].product = prod;
            }
        }
    }

    return items;
}
