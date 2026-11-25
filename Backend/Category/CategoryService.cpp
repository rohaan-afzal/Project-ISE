#include "CategoryService.h"
#include <cctype>

CategoryService::CategoryService(const std::string& categoryFilePath)
    : repository(categoryFilePath)
{
}

bool CategoryService::isValidCategoryID(const std::string& categoryID)
{
    if (categoryID.size() != 2)
        return false;
    if (categoryID[0] < '1' || categoryID[0] > '9')
        return false;
    return std::isdigit(static_cast<unsigned char>(categoryID[1])) != 0;
}

std::vector<ProductCategory> CategoryService::getAllCategories() const
{
    return repository.getAllCategories();
}

bool CategoryService::getCategoryByID(const std::string& categoryID, ProductCategory& category) const
{
    if (!isValidCategoryID(categoryID))
        return false;
    return repository.getCategoryByID(categoryID, category);
}

bool CategoryService::addCategory(const ProductCategory& category) const
{
    if (!isValidCategoryID(category.categoryID))
        return false;
    if (category.categoryName.empty() || category.categoryAcronym.empty())
        return false;

    ProductCategory existing;
    if (repository.getCategoryByID(category.categoryID, existing))
        return false;

    return repository.addCategory(category);
}

bool CategoryService::updateCategory(const ProductCategory& category) const
{
    if (!isValidCategoryID(category.categoryID))
        return false;
    if (category.categoryName.empty() || category.categoryAcronym.empty())
        return false;

    std::vector<ProductCategory> categories = repository.getAllCategories();
    bool found = false;
    for (auto& c : categories)
    {
        if (c.categoryID == category.categoryID)
        {
            c = category;
            found = true;
            break;
        }
    }
    return found && repository.saveAllCategories(categories);
}

std::string CategoryService::getNextCategoryID() const
{
    int highest = 10;
    for (const auto& c : repository.getAllCategories())
    {
        try
        {
            int val = std::stoi(c.categoryID);
            if (val >= highest)
                highest = val + 1;
        }
        catch (...)
        {
        }
    }
    if (highest > 99)
        highest = 10;
    return std::to_string(highest);
}