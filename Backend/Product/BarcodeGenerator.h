#pragma once

#include <string>
#include <vector>

class BarcodeGenerator {
public:
    virtual ~BarcodeGenerator() = default;
    virtual std::string generate(const std::string& categoryID,
                                 const std::vector<std::string>& existingBarcodes) const = 0;
};
