#include "ReportService.h"
#include "../Billing/BillRepo3.h"
#include "../Product/ProductService.h"
#include "../Backend/Billing/Record-Format.h"
#include <fstream>
#include <iomanip>

ReportService::ReportService(const std::string& billPath,
    const std::string& billDetailPath,
    const std::string& productPath)
    : m_billPath(billPath),
    m_billDetailPath(billDetailPath),
    m_productPath(productPath)
{
}

int ReportService::dateToComparableInt(const std::string& dmy)
{
    if (dmy.size() != 10 || dmy[2] != '-' || dmy[5] != '-')
        return -1;

    int d = 0, m = 0, y = 0;
    if (!RecordFormat::toInt(dmy.substr(0, 2), d) ||
        !RecordFormat::toInt(dmy.substr(3, 2), m) ||
        !RecordFormat::toInt(dmy.substr(6, 4), y))
        return -1;

    return y * 10000 + m * 100 + d;
}

std::vector<Bill> ReportService::getFilteredBills(const std::string& startDateDMY,
    const std::string& endDateDMY,
    bool filterByDate) const
{
    FileBillRepository billRepo(m_billPath);
    FileBillDetailRepository detailRepo(m_billDetailPath);

    const std::vector<Bill> allBills = billRepo.getAllBills();
    const int startKey = filterByDate ? dateToComparableInt(startDateDMY) : -1;
    const int endKey = filterByDate ? dateToComparableInt(endDateDMY) : -1;

    std::vector<Bill> result;
    for (Bill b : allBills)
    {
        if (filterByDate)
        {
            const int billKey = dateToComparableInt(b.billDate);
            if (billKey < 0)
                continue;
            if (startKey > 0 && billKey < startKey)
                continue;
            if (endKey > 0 && billKey > endKey)
                continue;
        }
        b.lines = detailRepo.getLinesByBillID(b.billID);
        result.push_back(b);
    }
    return result;
}

SalesSummary ReportService::computeSalesSummary(const std::vector<Bill>& bills) const
{
    SalesSummary summary;
    summary.billCount = static_cast<int>(bills.size());

    for (const auto& b : bills)
    {
        for (const auto& line : b.lines)
            summary.totalItemsSold += line.quantity;

        summary.totalGrossWithoutTax += b.totalWithoutTax;
        summary.totalTax += b.totalTax;
        summary.totalProductDiscount += b.totalDiscount;
        summary.totalSeasonalDiscount += b.seasonalDiscount;
        summary.totalSpecialDiscount += b.specialDiscount;
        summary.totalNetRevenue += b.totalPayable;
    }

    return summary;
}

std::vector<Product> ReportService::getAllProducts() const
{
    ProductService prodService(m_productPath);
    return prodService.getAllProducts();
}

InventorySummary ReportService::computeInventorySummary(const std::vector<Product>& products,
    int lowStockThreshold) const
{
    InventorySummary summary;
    summary.totalSkus = static_cast<int>(products.size());

    for (const auto& p : products)
    {
        summary.totalUnitsInStock += p.quantity;
        summary.totalStockValuation += (p.unitPrice * p.quantity);
        if (p.quantity <= lowStockThreshold)
            ++summary.lowStockCount;
    }

    return summary;
}

bool ReportService::exportSalesCsv(const std::string& outFilePath,
    const std::vector<Bill>& bills,
    const SalesSummary& summary) const
{
    std::ofstream out(outFilePath.c_str(), std::ios::trunc);
    if (!out.is_open())
        return false;

    out << std::fixed << std::setprecision(2);
    out << "BillID,OrderID,Customer,Date,Time,ItemsCount,GrossWithoutTax,Tax,ProductDiscount,SeasonalDiscount,SpecialDiscount,TotalPayable,Status\n";

    for (const auto& b : bills)
    {
        int itemCount = 0;
        for (const auto& l : b.lines)
            itemCount += l.quantity;

        out << b.billID << ","
            << b.orderID << ","
            << b.customerName << ","
            << b.billDate << ","
            << b.billTime << ","
            << itemCount << ","
            << b.totalWithoutTax << ","
            << b.totalTax << ","
            << b.totalDiscount << ","
            << b.seasonalDiscount << ","
            << b.specialDiscount << ","
            << b.totalPayable << ","
            << b.paymentStatus << "\n";
    }

    out << "\nSUMMARY,,,Bills: " << summary.billCount
        << ",Items Sold: " << summary.totalItemsSold
        << ",Gross: " << summary.totalGrossWithoutTax
        << ",Tax: " << summary.totalTax
        << ",ProdDisc: " << summary.totalProductDiscount
        << ",SeasonalDisc: " << summary.totalSeasonalDiscount
        << ",SpecialDisc: " << summary.totalSpecialDiscount
        << ",NetRevenue: " << summary.totalNetRevenue << "\n";

    return out.good();
}

bool ReportService::exportInventoryCsv(const std::string& outFilePath,
    const std::vector<Product>& products,
    int lowStockThreshold,
    const InventorySummary& summary) const
{
    std::ofstream out(outFilePath.c_str(), std::ios::trunc);
    if (!out.is_open())
        return false;

    out << std::fixed << std::setprecision(2);
    out << "ProductID,CategoryID,Barcode,Acronym,Name,Unit,UnitPrice,Tax,Discount,StockQty,StockValue,Status\n";

    for (const auto& p : products)
    {
        const double stockVal = p.unitPrice * p.quantity;
        const std::string status = (p.quantity == 0) ? "OUT_OF_STOCK"
            : (p.quantity <= lowStockThreshold) ? "LOW_STOCK"
            : "OK";
        out << p.productID << ","
            << p.categoryID << ","
            << p.barcode << ","
            << p.acronym << ","
            << p.name << ","
            << p.unit << ","
            << p.unitPrice << ","
            << p.tax << ","
            << p.discount << ","
            << p.quantity << ","
            << stockVal << ","
            << status << "\n";
    }

    out << "\nSUMMARY,,,TotalSKUs: " << summary.totalSkus
        << ",TotalUnits: " << summary.totalUnitsInStock
        << ",LowStockItems: " << summary.lowStockCount
        << ",TotalValuation: " << summary.totalStockValuation << "\n";

    return out.good();
}