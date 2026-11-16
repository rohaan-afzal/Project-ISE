#pragma once

#include "Product.h"
#include <optional>
#include <string>

class ProductService {
public:
    explicit ProductService(std::string catalogPath);
    std::optional<Product> findByBarcode(const std::string& barcode) const;
private:
    std::string m_catalogPath;
};
