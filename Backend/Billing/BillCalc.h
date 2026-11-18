#pragma once
#ifndef BILLCALC_H
#define BILLCALC_H

#include <string>
#include <vector>
#include "Bill.h"
#include "BillRepo.h"

class PricingService
{
public:
    static double roundMoney(double value);
    static double getLineSubtotal(const BillLine& line);
    static double getLineTax(const BillLine& line);
    static double getLineDiscount(const BillLine& line);
    static double getLineTotal(const BillLine& line);

    static double getTotalWithoutTax(const std::vector<BillLine>& lines);
    static double getTotalTax(const std::vector<BillLine>& lines);
    static double getTotalDiscount(const std::vector<BillLine>& lines);
    static double getTotalPayable(const std::vector<BillLine>& lines);
};

class BillService
{
private:
    BillRepository& repository;

public:
    explicit BillService(BillRepository& repository);

    static bool isValidStatus(const std::string& status);
    static void validateInput(const std::string& customerName, const std::string& orderID,
                              const std::string& paymentStatus, const std::vector<BillLine>& lines);

    std::string getNextBillID() const;
    Bill createBill(const std::string& customerName, const std::string& orderID,
                    const std::string& paymentStatus, const std::vector<BillLine>& lines) const;
    void saveBill(const Bill& bill);
};

#endif