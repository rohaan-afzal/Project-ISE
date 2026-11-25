#ifndef BILLCALC_H
#define BILLCALC_H

#include <string>
#include <vector>
#include "Bill3.h"
#include "BillRepo3.h"
using namespace std;

class PricingService
{
private:
    static double roundMoney(double value);

public:
    static double getLineSubtotal(const BillLine& line);
    static double getLineTax(const BillLine& line);
    static double getLineDiscount(const BillLine& line);
    static double getLineAmount(const BillLine& line);

    static double getTotalWithoutTax(const vector<BillLine>& lines);
    static double getTotalTax(const vector<BillLine>& lines);
    static double getTotalDiscount(const vector<BillLine>& lines);
    static double getAmountAfterProductDiscount(const vector<BillLine>& lines);

    static double getPercentAmount(double amount, double percent);
    static double getTotalPayable(double amountAfterProductDiscount, double seasonalDiscount, double specialDiscount);
    static double getBalanceReturned(double amountReceived, double totalPayable);
};

struct SeasonRule
{
    string startDate;
    string endDate;
    double percentage;
    bool isActive;

    SeasonRule() : percentage(0.0), isActive(false)
    {
    }
};

class SeasonSource
{
public:
    virtual ~SeasonSource() {}

    virtual vector<SeasonRule> getSeasonRules() const = 0;
};

class SeasonalDiscountService
{
private:
    const SeasonSource& source;

    static int daysInMonth(int month, int year);
    static int dateToNumber(const string& date);

public:
    explicit SeasonalDiscountService(const SeasonSource& source);

    static bool isValidDate(const string& date);
    static bool isValidTime(const string& time);

    double getDiscountPercent(const string& date) const;
};

class BillService
{
private:
    BillRepository& billRepository;
    BillDetailRepository& detailRepository;
    const SeasonalDiscountService& seasonService;

    static bool isValidStatus(const string& status);
    static void validateRequest(const BillRequest& request);
    string getNextBillID() const;

public:
    BillService(BillRepository& billRepository, BillDetailRepository& detailRepository,
        const SeasonalDiscountService& seasonService);

    Bill createBill(const BillRequest& request) const;
    void saveBill(const Bill& bill);
    bool findBill(const string& billID, Bill& bill) const;

    static string getCurrentDate();
    static string getCurrentTime();
};

#endif