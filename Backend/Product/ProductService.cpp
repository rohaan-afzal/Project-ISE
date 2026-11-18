#include "ProductService.h"
#include <fstream>
#include <stdexcept>

ProductService::ProductService(std::string catalogPath) : m_catalogPath(std::move(catalogPath)) {}

std::optional<Product> ProductService::findByBarcode(const std::string& barcode) const {
    std::ifstream input(m_catalogPath);
    if (!input) throw std::runtime_error("Could not open the product catalog: " + m_catalogPath);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        Product product;
        const auto first = line.find('|');
        const auto second = first == std::string::npos ? first : line.find('|', first + 1);
        const auto third = second == std::string::npos ? second : line.find('|', second + 1);
        if (first == std::string::npos || second == std::string::npos || third == std::string::npos)
            continue;
        product.barcode = line.substr(0, first);
        product.name = line.substr(first + 1, second - first - 1);
        product.description = line.substr(second + 1, third - second - 1);
        product.specifications = line.substr(third + 1);
        if (product.barcode == barcode) return product;
    }
    return std::nullopt;
}
