#pragma once
#ifndef PRODUCTREPOSITORY_H
#define PRODUCTREPOSITORY_H

#include <string>
#include <vector>
#include "Product.h"

class ProductRepository
{
private:
    std::string filePath;

public:
    explicit ProductRepository(const std::string& path = "Data/Product.txt");

    bool getProductByBarcode(const std::string& barcode, Product& product) const;
    bool getProductByID(const std::string& productID, Product& product) const;
    bool addProduct(const Product& product) const;
    std::vector<Product> getAllProducts() const;
};

#endif

