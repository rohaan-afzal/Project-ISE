#include "CategoryRepo.h"
#include <fstream>
#include <sstream>

CategoryRepository::CategoryRepository(const std::string& path)
    : filePath(path)
{
}

std::vector<ProductCategory> CategoryRepository::getAllCategories() const
{
    std::vector<ProductCategory> categories;
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return categories;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        ProductCategory cat;
        std::string hashToken;

        if (!(iss >> cat.categoryID >> cat.categoryName >> cat.categoryAcronym >> hashToken))
            continue;

        if (hashToken != "#")
            continue;

        categories.push_back(cat);
    }

    return categories;
}

bool CategoryRepository::getCategoryByID(const std::string& categoryID, ProductCategory& category) const
{
    for (const auto& c : getAllCategories())
    {
        if (c.categoryID == categoryID)
        {
            category = c;
            return true;
        }
    }
    return false;
}

bool CategoryRepository::addCategory(const ProductCategory& category) const
{
    std::ofstream file(filePath.c_str(), std::ios::app);
    if (!file.is_open())
        return false;

    file << category.categoryID << " "
        << category.categoryName << " "
        << category.categoryAcronym << " #" << '\n';

    file.flush();
    const bool ok = !file.fail();
    file.close();
    return ok;
}

bool CategoryRepository::saveAllCategories(const std::vector<ProductCategory>& categories) const
{
    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        return false;

    for (const auto& category : categories)
    {
        file << category.categoryID << " "
            << category.categoryName << " "
            << category.categoryAcronym << " #" << '\n';
    }

    file.flush();
    const bool ok = !file.fail();
    file.close();
    return ok;
}