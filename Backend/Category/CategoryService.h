#pragma once
#ifndef CATEGORYSERVICE_H
#define CATEGORYSERVICE_H

#include <string>
#include <vector>
#include "ProductCategory.h"
#include "CategoryRepo.h"

class CategoryService
{
private:
    CategoryRepository repository;

public:
    explicit CategoryService(const std::string& categoryFilePath = "Data/PCategory.txt");

    std::vector<ProductCategory> getAllCategories() const;
    bool getCategoryByID(const std::string& categoryID, ProductCategory& category) const;
    bool addCategory(const ProductCategory& category) const;
    bool updateCategory(const ProductCategory& category) const;
    std::string getNextCategoryID() const;
    static bool isValidCategoryID(const std::string& categoryID);
};

#endif