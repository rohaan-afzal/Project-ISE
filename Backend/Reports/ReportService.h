#pragma once
#ifndef REPORTSERVICE_H
#define REPORTSERVICE_H

#include <string>
#include <vector>
#include "../Billing/Bill3.h"
#include "../Product/Product.h"

struct SalesSummary
{
    int billCount = 0;
    int totalItemsSold = 0;
    double totalGrossWithoutTax = 0.0;
    double totalTax = 0.0;
    double totalProductDiscount = 0.0;
    double totalSeasonalDiscount = 0.0;
    double totalSpecialDiscount = 0.0;
    double totalNetRevenue = 0.0;
};

struct InventorySummary
{
    int totalSkus = 0;
    int totalUnitsInStock = 0;
    int lowStockCount = 0;
    double totalStockValuation = 0.0;
};

class ReportService
{
public:
    ReportService(const std::string& billPath,
        const std::string& billDetailPath,
        const std::string& productPath);

    std::vector<Bill> getFilteredBills(const std::string& startDateDMY,
        const std::string& endDateDMY,
        bool filterByDate) const;

    SalesSummary computeSalesSummary(const std::vector<Bill>& bills) const;

    std::vector<Product> getAllProducts() const;
    InventorySummary computeInventorySummary(const std::vector<Product>& products,
        int lowStockThreshold) const;

    bool exportSalesCsv(const std::string& outFilePath,
        const std::vector<Bill>& bills,
        const SalesSummary& summary) const;

    bool exportInventoryCsv(const std::string& outFilePath,
        const std::vector<Product>& products,
        int lowStockThreshold,
        const InventorySummary& summary) const;

private:
    std::string m_billPath;
    std::string m_billDetailPath;
    std::string m_productPath;

    static int dateToComparableInt(const std::string& dmy);
};

#endif