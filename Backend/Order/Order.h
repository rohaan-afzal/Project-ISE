#pragma once
#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>
#include "Product.h"
#include "Payment.h"
#include "../Billing/Bill3.h"

struct OrderItem
{
    Product product;
    int quantity;

    OrderItem() : quantity(0) {}
    OrderItem(const Product& prod, int qty) : product(prod), quantity(qty) {}
};

struct Order
{
    std::string orderID;
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM:SS
    std::string customerName;
    std::string customerPhone;
    std::string status;       // "in-progress", "on-hold", "paid", "archived"
    Payment payment;
    std::vector<OrderItem> items;

    Order();
    std::vector<BillLine> toBillLines() const;
};

#endif