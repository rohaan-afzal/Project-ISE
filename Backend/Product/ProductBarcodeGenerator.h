#pragma once

#include "BarcodeGenerator.h"

class ProductBarcodeGenerator final : public BarcodeGenerator {
public:
    // Category IDs are exactly two digits; the remaining four digits are allocated
    // from the unused codes in that category.
    std::string generate(const std::string& categoryID,
                         const std::vector<std::string>& existingBarcodes) const override;
};
