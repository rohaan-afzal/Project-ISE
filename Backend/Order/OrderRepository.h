#pragma once
#ifndef ORDERREPOSITORY_H
#define ORDERREPOSITORY_H

#include <string>
#include <vector>
#include "Order.h"
#include "ProductService.h"

class OrderRepository
{
private:
    std::string filePath;

public:
    explicit OrderRepository(const std::string& path = "Data/Order.txt");

    std::vector<Order> getAllOrders() const;
    void saveOrder(const Order& order) const;
    void saveAllOrders(const std::vector<Order>& orders) const;
};

class OrderDetailRepository
{
private:
    std::string filePath;

public:
    explicit OrderDetailRepository(const std::string& path = "Data/OrderDetail.txt");

    void saveOrderDetails(const std::string& orderID, const std::vector<OrderItem>& items) const;
    std::vector<OrderItem> getDetailsForOrder(const std::string& orderID, const ProductService& productService) const;
};

#endif