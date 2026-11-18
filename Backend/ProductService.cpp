#include "ProductService.h"

ProductService::ProductService(const std::string& productFilePath)
    : repository(productFilePath)
{
}

bool ProductService::getProductByBarcode(const std::string& barcode, Product& product) const
{
    if (barcode.length() != 6)
    {
        return false;
    }

    for (std::size_t i = 0; i < barcode.length(); i++)
    {
        if (barcode[i] < '0' || barcode[i] > '9')
        {
            return false;
        }
    }

    return repository.getProductByBarcode(barcode, product);
}

bool ProductService::getProductByID(const std::string& productID, Product& product) const
{
    if (productID.empty())
    {
        return false;
    }

    return repository.getProductByID(productID, product);
}

bool ProductService::addProduct(const Product& product) const
{
    if (product.productID.empty() ||
        product.barcode.length() != 6 ||
        product.acronym.empty() ||
        product.name.empty() ||
        product.unit.empty() ||
        product.unitPrice < 0 ||
        product.tax < 0 ||
        product.discount < 0 ||
        product.quantity < 0)
    {
        return false;
    }

    Product existing;

    if (repository.getProductByBarcode(product.barcode, existing))
    {
        return false;
    }

    if (repository.getProductByID(product.productID, existing))
    {
        return false;
    }

    return repository.addProduct(product);
}
