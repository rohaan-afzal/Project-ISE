#include "ProductService.h"
#include "../Order/Input-Validator.h"
#include "../Category/CategoryService.h"

ProductService::ProductService(const std::string& productFilePath)
    : repository(productFilePath)
{
}

bool ProductService::getProductByBarcode(const std::string& barcode, Product& product) const
{
    if (!InputValidator::isValidBarcode(barcode))
        return false;
    return repository.getProductByBarcode(barcode, product);
}

bool ProductService::getProductByID(const std::string& productID, Product& product) const
{
    if (!InputValidator::isValidID(productID))
        return false;
    return repository.getProductByID(productID, product);
}

bool ProductService::addProduct(const Product& product) const
{
    if (!InputValidator::isValidID(product.productID))
        return false;
    if (!CategoryService::isValidCategoryID(product.categoryID))
        return false;
    if (!InputValidator::isValidBarcode(product.barcode))
        return false;
    if (product.barcode.substr(0, 2) != product.categoryID)
        return false;
    if (product.acronym.empty() || product.name.empty())
        return false;
    if (!InputValidator::isValidUnit(product.unit))
        return false;
    if (product.unitPrice < 0.0 || product.tax < 0.0 || product.discount < 0.0)
        return false;
    if (product.discount > product.unitPrice)
        return false;
    if (product.quantity < 0)
        return false;

    Product existing;
    if (repository.getProductByID(product.productID, existing))
        return false;
    if (repository.getProductByBarcode(product.barcode, existing))
        return false;

    return repository.addProduct(product);
}

bool ProductService::updateProduct(const Product& product) const
{
    if (!InputValidator::isValidID(product.productID))
        return false;
    if (!CategoryService::isValidCategoryID(product.categoryID))
        return false;
    if (!InputValidator::isValidBarcode(product.barcode))
        return false;
    if (product.barcode.substr(0, 2) != product.categoryID)
        return false;
    if (product.acronym.empty() || product.name.empty())
        return false;
    if (!InputValidator::isValidUnit(product.unit))
        return false;
    if (product.unitPrice < 0.0 || product.tax < 0.0 || product.discount < 0.0)
        return false;
    if (product.discount > product.unitPrice)
        return false;
    if (product.quantity < 0)
        return false;

    Product existingByID;
    if (!repository.getProductByID(product.productID, existingByID))
        return false;

    Product existingByBarcode;
    if (repository.getProductByBarcode(product.barcode, existingByBarcode) &&
        existingByBarcode.productID != product.productID)
    {
        return false;
    }

    return repository.updateProduct(product);
}

bool ProductService::deductStock(const std::string& barcode, int soldQuantity) const
{
    if (soldQuantity <= 0)
        return false;

    Product existing;
    if (!repository.getProductByBarcode(barcode, existing))
        return false;

    if (existing.quantity < soldQuantity)
        return false;

    return repository.updateStock(barcode, existing.quantity - soldQuantity);
}

bool ProductService::restockProduct(const std::string& barcode, int addedQuantity) const
{
    if (addedQuantity <= 0)
        return false;

    Product existing;
    if (!repository.getProductByBarcode(barcode, existing))
        return false;

    return repository.updateStock(barcode, existing.quantity + addedQuantity);
}

bool ProductService::removeProduct(const std::string& barcode) const
{
    if (barcode.empty())
        return false;

    return repository.removeProduct(barcode);
}

std::vector<Product> ProductService::getAllProducts() const
{
    return repository.getAllProducts();
}

std::string ProductService::getNextProductID() const
{
    int highest = 1000;
    const std::vector<Product> products = repository.getAllProducts();
    for (const auto& p : products)
    {
        try
        {
            std::size_t pos = 0;
            int id = std::stoi(p.productID, &pos);
            if (pos == p.productID.size() && id > highest)
                highest = id;
        }
        catch (...)
        {
        }
    }
    return std::to_string(highest + 1);
}