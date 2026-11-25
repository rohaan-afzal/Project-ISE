#pragma once
#ifndef CATEGORYREPOSITORY_H
#define CATEGORYREPOSITORY_H

#include <string>
#include <vector>
#include "ProductCategory.h"

class CategoryRepository
{
private:
    std::string filePath;

public:
    explicit CategoryRepository(const std::string& path = "Data/PCategory.txt");

    std::vector<ProductCategory> getAllCategories() const;
    bool getCategoryByID(const std::string& categoryID, ProductCategory& category) const;
    bool addCategory(const ProductCategory& category) const;
    bool saveAllCategories(const std::vector<ProductCategory>& categories) const;
};

#endif