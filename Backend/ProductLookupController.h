#pragma once
#ifndef PRODUCTLOOKUPCONTROLLER_H
#define PRODUCTLOOKUPCONTROLLER_H

#include <string>
#include "ProductService.h"

class ProductDescriptionView;

class ProductLookupController
{
private:
    ProductService* productService;
    ProductDescriptionView* descriptionView;

public:
    ProductLookupController(ProductService* service, ProductDescriptionView* view);

    void showProductDescription(const std::string& barcode);
};

#endif
