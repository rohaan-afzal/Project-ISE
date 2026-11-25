#include "InventoryService.h"
#include "../Product/ProductService.h"
#include <map>
#include <stdexcept>

void InventoryService::applyOrderChange(const std::vector<OrderItem>& oldItems,
                                        const std::vector<OrderItem>& newItems) const
{
    std::map<std::string, int> oldQuantities;
    std::map<std::string, int> newQuantities;
    for (const auto& item : oldItems) oldQuantities[item.product.barcode] += item.quantity;
    for (const auto& item : newItems) newQuantities[item.product.barcode] += item.quantity;

    std::map<std::string, int> deltas;
    for (const auto& entry : oldQuantities) deltas[entry.first] -= entry.second;
    for (const auto& entry : newQuantities) deltas[entry.first] += entry.second;

    // Validate the complete change before touching any stock.
    for (const auto& entry : deltas) {
        if (entry.second <= 0) continue;
        Product product;
        if (!productService.getProductByBarcode(entry.first, product))
            throw std::runtime_error("Product no longer exists for barcode: " + entry.first);
        if (product.quantity < entry.second)
            throw std::runtime_error("Insufficient stock for " + product.name +
                ". Available: " + std::to_string(product.quantity) +
                ", requested additional quantity: " + std::to_string(entry.second) + ".");
    }

    std::vector<std::pair<std::string, int>> applied;
    for (const auto& entry : deltas) {
        if (entry.second == 0) continue;
        const bool changed = entry.second > 0
            ? productService.deductStock(entry.first, entry.second)
            : productService.restockProduct(entry.first, -entry.second);
        if (!changed) {
            for (auto it = applied.rbegin(); it != applied.rend(); ++it) {
                if (it->second > 0) productService.restockProduct(it->first, it->second);
                else productService.deductStock(it->first, -it->second);
            }
            throw std::runtime_error("Could not update stock for barcode: " + entry.first);
        }
        applied.emplace_back(entry.first, entry.second);
    }
}
