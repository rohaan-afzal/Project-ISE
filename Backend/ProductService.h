#pragma once
#ifndef PRODUCTSERVICE_H
#define PRODUCTSERVICE_H

#include <string>
#include "Product.h"
#include "ProductRepository.h"

class ProductService
{
private:
    ProductRepository repository;

public:
    explicit ProductService(const std::string& productFilePath = "Backend/Product.txt");

    bool getProductByBarcode(const std::string& barcode, Product& product) const;
    bool getProductByID(const std::string& productID, Product& product) const;
    bool addProduct(const Product& product) const;
};

#endif
