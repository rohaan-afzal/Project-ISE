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

        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok)
            tokens.push_back(tok);

        if (tokens.empty() || tokens.back() != "#")
        {
            // Legacy support: older rows with # before image path
            if (tokens.size() >= 10 && tokens[9] == "#")
            {
                try
                {
                    Product p;
                    p.productID = tokens[0];
                    p.barcode = tokens[1];
                    p.categoryID = (p.barcode.size() >= 2) ? p.barcode.substr(0, 2) : "10";
                    p.acronym = tokens[2];
                    p.name = tokens[3];
                    p.unit = tokens[4];
                    p.unitPrice = std::stod(tokens[5]);
                    p.tax = std::stod(tokens[6]);
                    p.discount = std::stod(tokens[7]);
                    p.quantity = std::stoi(tokens[8]);
                    if (tokens.size() >= 11)
                        p.barcodeImagePath = tokens[10];
                    products.push_back(p);
                }
                catch (...) {}
            }
            continue;
        }

        // New format (# always at the very end):
        // With image path (12 tokens): ID CatID Barcode Acronym Name Unit Price Tax Discount Qty ImagePath #
        // Without image path (11 tokens): ID CatID Barcode Acronym Name Unit Price Tax Discount Qty #
        if (tokens.size() == 12 || tokens.size() == 11)
        {
            try
            {
                Product p;
                p.productID = tokens[0];
                p.categoryID = tokens[1];
                p.barcode = tokens[2];
                p.acronym = tokens[3];
                p.name = tokens[4];
                p.unit = tokens[5];
                p.unitPrice = std::stod(tokens[6]);
                p.tax = std::stod(tokens[7]);
                p.discount = std::stod(tokens[8]);
                p.quantity = std::stoi(tokens[9]);
                if (tokens.size() == 12)
                    p.barcodeImagePath = tokens[10];
                products.push_back(p);
            }
            catch (...) {}
        }
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
         << product.categoryID << " "
         << product.barcode << " "
         << product.acronym << " "
         << product.name << " "
         << product.unit << " "
         << product.unitPrice << " "
         << product.tax << " "
         << product.discount << " "
         << product.quantity;

    if (!product.barcodeImagePath.empty())
        file << " " << product.barcodeImagePath;

    file << " #" << '\n';
    file.flush();
    const bool ok = !file.fail();
    file.close();
    return ok;
}

bool ProductRepository::updateProduct(const Product& updatedProduct) const
{
    std::vector<Product> products = getAllProducts();
    bool found = false;
    for (auto& p : products)
    {
        if (p.productID == updatedProduct.productID)
        {
            p = updatedProduct;
            found = true;
            break;
        }
    }
    return found && saveAllProducts(products);
}

bool ProductRepository::saveAllProducts(const std::vector<Product>& products) const
{
    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        return false;

    for (const auto& product : products)
    {
        file << product.productID << " "
             << product.categoryID << " "
             << product.barcode << " "
             << product.acronym << " "
             << product.name << " "
             << product.unit << " "
             << product.unitPrice << " "
             << product.tax << " "
             << product.discount << " "
             << product.quantity;

        if (!product.barcodeImagePath.empty())
            file << " " << product.barcodeImagePath;

        file << " #" << '\n';
    }

    file.flush();
    const bool ok = !file.fail();
    file.close();
    return ok;
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