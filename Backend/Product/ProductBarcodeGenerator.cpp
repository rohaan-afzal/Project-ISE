#include "ProductBarcodeGenerator.h"
#include <stdexcept>
#include <unordered_set>

std::string ProductBarcodeGenerator::generate(
    const std::string& categoryID,
    const std::vector<std::string>& existingBarcodes) const
{
    if (categoryID.size() != 2 || categoryID[0] < '1' || categoryID[0] > '9' ||
        categoryID[1] < '0' || categoryID[1] > '9')
        throw std::invalid_argument("Category ID must be a two-digit number from 10 to 99.");

    const std::unordered_set<std::string> used(existingBarcodes.begin(), existingBarcodes.end());
    for (int suffix = 0; suffix <= 9999; ++suffix) {
        std::string barcode = categoryID;
        const std::string digits = std::to_string(suffix);
        barcode.append(4 - digits.size(), '0');
        barcode += digits;
        if (used.find(barcode) == used.end())
            return barcode;
    }
    throw std::runtime_error("No barcode codes remain for this category.");
}
