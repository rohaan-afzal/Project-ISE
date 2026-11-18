#include "ProductRepository.h"
#include <fstream>
#include <sstream>

ProductRepository::ProductRepository(const std::string& path)
    : filePath(path)
{
}

std::vector<Product> ProductRepository::getAllProducts() const
{
    std::vector<Product> products;
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return products;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        Product p;
        std::string hashToken;

        if (!(iss >> p.productID
              >> p.barcode
              >> p.acronym
              >> p.name
              >> p.unit
              >> p.unitPrice
              >> p.tax
              >> p.discount
              >> p.quantity
              >> hashToken))
        {
            continue;
        }

        if (hashToken != "#")
            continue;

        std::string imgPath;
        if (iss >> imgPath)
            p.barcodeImagePath = imgPath;

        products.push_back(p);
    }

    return products;
}

bool ProductRepository::getProductByBarcode(const std::string& barcode, Product& product) const
{
    for (const auto& p : getAllProducts())
    {
        if (p.barcode == barcode)
        {
            product = p;
            return true;
        }
    }
    return false;
}

bool ProductRepository::getProductByID(const std::string& productID, Product& product) const
{
    for (const auto& p : getAllProducts())
    {
        if (p.productID == productID)
        {
            product = p;
            return true;
        }
    }
    return false;
}

bool ProductRepository::addProduct(const Product& product) const
{
    std::ofstream file(filePath.c_str(), std::ios::app);
    if (!file.is_open())
        return false;

    file << product.productID << " "
         << product.barcode << " "
         << product.acronym << " "
         << product.name << " "
         << product.unit << " "
         << product.unitPrice << " "
         << product.tax << " "
         << product.discount << " "
         << product.quantity << " #";

    if (!product.barcodeImagePath.empty())
        file << " " << product.barcodeImagePath;

    file << std::endl;
    file.close();
    return file.good();
}

bool ProductRepository::saveAllProducts(const std::vector<Product>& products) const
{
    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        return false;

    for (const auto& product : products)
    {
        file << product.productID << " "
             << product.barcode << " "
             << product.acronym << " "
             << product.name << " "
             << product.unit << " "
             << product.unitPrice << " "
             << product.tax << " "
             << product.discount << " "
             << product.quantity << " #";

        if (!product.barcodeImagePath.empty())
            file << " " << product.barcodeImagePath;

        file << std::endl;
    }

    file.close();
    return file.good();
}

bool ProductRepository::updateStock(const std::string& barcode, int newQuantity) const
{
    if (newQuantity < 0)
        return false;

    std::vector<Product> products = getAllProducts();
    bool found = false;
    for (auto& product : products)
    {
        if (product.barcode == barcode)
        {
            product.quantity = newQuantity;
            found = true;
            break;
        }
    }

    return found && saveAllProducts(products);
}

bool ProductRepository::removeProduct(const std::string& barcode) const
{
    std::vector<Product> products = getAllProducts();
    std::vector<Product> remaining;
    remaining.reserve(products.size());

    bool removed = false;
    for (const auto& product : products)
    {
        if (product.barcode == barcode)
        {
            removed = true;
            continue;
        }
        remaining.push_back(product);
    }

    return removed && saveAllProducts(remaining);
}