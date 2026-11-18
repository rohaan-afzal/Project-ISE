#pragma once
#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

struct Product
{
    std::string productID;
    std::string barcode;
    std::string acronym;
    std::string name;
    std::string unit;
    double unitPrice;
    double tax;
    double discount;
    int quantity;
    std::string barcodeImagePath;

    Product()
        : unitPrice(0.0), tax(0.0), discount(0.0), quantity(0)
    {
    }
};

#endif
