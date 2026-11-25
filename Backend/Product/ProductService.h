#pragma once
#ifndef PRODUCTSERVICE_H
#define PRODUCTSERVICE_H

#include <string>
#include <vector>
#include "Product.h"
#include "ProductRepository.h"

class ProductService
{
private:
    ProductRepository repository;

public:
    explicit ProductService(const std::string& productFilePath = "Data/Product.txt");

    bool getProductByBarcode(const std::string& barcode, Product& product) const;
    bool getProductByID(const std::string& productID, Product& product) const;
    bool updateProduct(const Product& product) const;
    bool addProduct(const Product& product) const;
    bool deductStock(const std::string& barcode, int soldQuantity) const;
    bool restockProduct(const std::string& barcode, int addedQuantity) const;
    bool removeProduct(const std::string& barcode) const;
    std::vector<Product> getAllProducts() const;
    std::string getNextProductID() const;
};

#endif