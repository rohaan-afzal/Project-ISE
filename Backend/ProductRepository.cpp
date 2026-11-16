#include "ProductRepository.h"
#include <fstream>
#include <sstream>

ProductRepository::ProductRepository(const std::string& path)
{
    filePath = path;
}

std::vector<Product> ProductRepository::getAllProducts() const
{
    std::vector<Product> products;
    std::ifstream file(filePath.c_str());

    if (!file.is_open())
    {
        return products;
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::istringstream input(line);
        Product product;
        std::string marker;

        if (!(input >> product.productID
            >> product.barcode
            >> product.acronym
            >> product.name
            >> product.unit
            >> product.unitPrice
            >> product.tax
            >> product.discount
            >> product.quantity
            >> marker))
        {
            continue;
        }

        if (marker != "#")
        {
            continue;
        }

        input >> product.barcodeImagePath;
        products.push_back(product);
    }

    file.close();
    return products;
}

bool ProductRepository::getProductByBarcode(const std::string& barcode, Product& product) const
{
    std::vector<Product> products = getAllProducts();

    for (std::size_t i = 0; i < products.size(); i++)
    {
        if (products[i].barcode == barcode)
        {
            product = products[i];
            return true;
        }
    }

    return false;
}

bool ProductRepository::getProductByID(const std::string& productID, Product& product) const
{
    std::vector<Product> products = getAllProducts();

    for (std::size_t i = 0; i < products.size(); i++)
    {
        if (products[i].productID == productID)
        {
            product = products[i];
            return true;
        }
    }

    return false;
}

bool ProductRepository::addProduct(const Product& product) const
{
    std::ofstream file(filePath.c_str(), std::ios::app);

    if (!file.is_open())
    {
        return false;
    }

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
    {
        file << " " << product.barcodeImagePath;
    }

    file << std::endl;
    file.close();

    return file.good();
}
