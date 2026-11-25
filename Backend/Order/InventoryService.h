#pragma once

#include "Order.h"

class ProductService;

// Applies the net stock delta between two versions of an order.
class InventoryService {
public:
    explicit InventoryService(ProductService& products) : productService(products) {}

    void applyOrderChange(const std::vector<OrderItem>& oldItems,
                          const std::vector<OrderItem>& newItems) const;

private:
    ProductService& productService;
};
