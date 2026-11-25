#include "BillCalc3.h"
#include "Record-Format.h"
#include <cmath>
#include <ctime>
#include <stdexcept>
using namespace std;

static int billIDLength = 9;
static double maxSpecialDiscountPercent = 8.0;
static const string validStatuses[4] = { "in-progress", "on-hold", "paid", "archived" };

double PricingService::roundMoney(double value)
{
    return floor(value * 100.0 + 0.5) / 100.0;
}

double PricingService::getLineSubtotal(const BillLine& line)
{
    return roundMoney(line.unitPrice * line.quantity);
}

double PricingService::getLineTax(const BillLine& line)
{
    return roundMoney(line.unitTax * line.quantity);
}

double PricingService::getLineDiscount(const BillLine& line)
{
    return roundMoney(line.discount);
}

double PricingService::getLineAmount(const BillLine& line)
{
    return roundMoney(getLineSubtotal(line) - getLineDiscount(line));
}

double PricingService::getTotalWithoutTax(const vector<BillLine>& lines)
{
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineSubtotal(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getTotalTax(const vector<BillLine>& lines)
{
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineTax(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getTotalDiscount(const vector<BillLine>& lines)
{
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineDiscount(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getAmountAfterProductDiscount(const vector<BillLine>& lines)
{
    return roundMoney(getTotalWithoutTax(lines) + getTotalTax(lines) - getTotalDiscount(lines));
}


double PricingService::getPercentAmount(double amount, double percent)
{
    return roundMoney(amount * percent / 100.0);
}

double PricingService::getTotalPayable(double amountAfterProductDiscount, double seasonalDiscount, double specialDiscount)
{
    return roundMoney(amountAfterProductDiscount - seasonalDiscount - specialDiscount);
}

double PricingService::getBalanceReturned(double amountReceived, double totalPayable)
{
    if (amountReceived <= totalPayable)
    {
        return 0.0;
    }

    return roundMoney(amountReceived - totalPayable);
}

SeasonalDiscountService::SeasonalDiscountService(const SeasonSource& source) : source(source)
{
}


int SeasonalDiscountService::daysInMonth(int month, int year)
{
    if (month == 2)
    {
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return isLeapYear ? 29 : 28;
    }

    if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        return 30;
    }

    return 31;
}

bool SeasonalDiscountService::isValidDate(const string& date)
{
    if (date.length() != 10 || date[2] != '-' || date[5] != '-')
    {
        return false;
    }

    for (int i = 0; i < 10; i++)
    {
        if (i != 2 && i != 5 && (date[i] < '0' || date[i] > '9'))
        {
            return false;
        }
    }

    int day = stoi(date.substr(0, 2));
    int month = stoi(date.substr(3, 2));
    int year = stoi(date.substr(6, 4));

    return month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth(month, year);
}

bool SeasonalDiscountService::isValidTime(const string& time)
{
    if (time.length() != 8 || time[2] != ':' || time[5] != ':')
    {
        return false;
    }

    for (int i = 0; i < 8; i++)
    {
        if (i != 2 && i != 5 && (time[i] < '0' || time[i] > '9'))
        {
            return false;
        }
    }

    int hour = stoi(time.substr(0, 2));
    int minute = stoi(time.substr(3, 2));
    int second = stoi(time.substr(6, 2));

    return hour < 24 && minute < 60 && second < 60;
}

int SeasonalDiscountService::dateToNumber(const string& date)
{
    if (!isValidDate(date))
    {
        return -1;
    }

    return stoi(date.substr(6, 4)) * 10000 + stoi(date.substr(3, 2)) * 100 + stoi(date.substr(0, 2));
}

double SeasonalDiscountService::getDiscountPercent(const string& date) const
{
    int today = dateToNumber(date);

    if (today < 0)
    {
        throw invalid_argument("Bill date must be a valid date written as DD-MM-YYYY.");
    }

    vector<SeasonRule> seasons = source.getSeasonRules();
    double best = 0.0;

    for (int i = 0; i < (int)seasons.size(); i++)
    {
        int start = dateToNumber(seasons[i].startDate);
        int end = dateToNumber(seasons[i].endDate);
        bool isUsable = seasons[i].isActive && start >= 0 && end >= 0 &&
            seasons[i].percentage >= 0 && seasons[i].percentage <= 100;

        if (isUsable && today >= start && today <= end && seasons[i].percentage > best)
        {
            best = seasons[i].percentage;
        }
    }

    return best;
}

BillService::BillService(BillRepository& billRepository, BillDetailRepository& detailRepository,
    const SeasonalDiscountService& seasonService)
    : billRepository(billRepository), detailRepository(detailRepository), seasonService(seasonService)
{
}

bool BillService::isValidStatus(const string& status)
{
    for (int i = 0; i < 4; i++)
    {
        if (validStatuses[i] == status)
        {
            return true;
        }
    }

    return false;
}

void BillService::validateRequest(const BillRequest& request)
{
    if (request.customerName.empty())
    {
        throw invalid_argument("Customer name is required.");
    }

    if (request.orderID.empty())
    {
        throw invalid_argument("Order ID is required.");
    }

    if (!isValidStatus(request.paymentStatus))
    {
        throw invalid_argument("Payment status must be in-progress, on-hold, paid or archived.");
    }

    if (!SeasonalDiscountService::isValidDate(request.billDate))
    {
        throw invalid_argument("Bill date must be a valid date written as DD-MM-YYYY.");
    }

    if (!SeasonalDiscountService::isValidTime(request.billTime))
    {
        throw invalid_argument("Bill time must be written as HH:MM:SS.");
    }

    if (request.amountReceived < 0)
    {
        throw invalid_argument("Amount received cannot be negative.");
    }

    if (request.specialDiscountPercent < 0 || request.specialDiscountPercent > maxSpecialDiscountPercent)
    {
        throw invalid_argument("Special discount must be between 0% and 8%.");
    }

    if (request.lines.empty())
    {
        throw invalid_argument("A bill needs at least one product.");
    }

    for (int i = 0; i < (int)request.lines.size(); i++)
    {
        const BillLine& line = request.lines[i];

        if (line.productAcronym.empty())
        {
            throw invalid_argument("Every bill line needs a product acronym.");
        }

        if (line.quantity <= 0)
        {
            throw invalid_argument("Quantity must be at least 1 for " + line.productAcronym + ".");
        }

        if (line.unitPrice < 0 || line.unitTax < 0)
        {
            throw invalid_argument("Price and tax cannot be negative for " + line.productAcronym + ".");
        }

        if (line.discount < 0 || line.discount > line.unitPrice * line.quantity)
        {
            throw invalid_argument("Discount must be between 0 and the price of the products for " + line.productAcronym + ".");
        }
    }
}

string BillService::getNextBillID() const
{
    vector<Bill> bills = billRepository.getAllBills();
    int highest = 0;

    for (int i = 0; i < (int)bills.size(); i++)
    {
        int id;

        if (RecordFormat::toInt(bills[i].billID, id) && id > highest)
        {
            highest = id;
        }
    }

    if (highest >= 999999999)
    {
        throw runtime_error("The bill ID limit has been reached.");
    }

    string nextID = to_string(highest + 1);

    while ((int)nextID.length() < billIDLength)
    {
        nextID = "0" + nextID;
    }

    return nextID;
}

Bill BillService::createBill(const BillRequest& request) const
{
    validateRequest(request);

    Bill bill;
    bill.billID = getNextBillID();
    bill.customerName = request.customerName;
    bill.orderID = request.orderID;
    bill.paymentStatus = request.paymentStatus;
    bill.billDate = request.billDate;
    bill.billTime = request.billTime;
    bill.lines = request.lines;

    for (int i = 0; i < (int)bill.lines.size(); i++)
    {
        bill.lines[i].billID = bill.billID;
    }

    double amountAfterDiscount = PricingService::getAmountAfterProductDiscount(bill.lines);
    double seasonPercent = seasonService.getDiscountPercent(bill.billDate);

    bill.totalWithoutTax = PricingService::getTotalWithoutTax(bill.lines);
    bill.totalTax = PricingService::getTotalTax(bill.lines);
    bill.totalDiscount = PricingService::getTotalDiscount(bill.lines);
    bill.seasonalDiscount = PricingService::getPercentAmount(amountAfterDiscount, seasonPercent);
    bill.specialDiscount = PricingService::getPercentAmount(amountAfterDiscount, request.specialDiscountPercent);
    bill.totalPayable = PricingService::getTotalPayable(amountAfterDiscount, bill.seasonalDiscount, bill.specialDiscount);

    if (bill.paymentStatus == "paid" && request.amountReceived < bill.totalPayable)
    {
        throw invalid_argument("Amount received is less than the amount payable.");
    }

    bill.amountReceived = request.amountReceived;
    bill.balanceReturned = PricingService::getBalanceReturned(bill.amountReceived, bill.totalPayable);

    return bill;
}

void BillService::saveBill(const Bill& bill)
{
    for (int i = 0; i < (int)bill.lines.size(); i++)
    {
        if (bill.lines[i].billID != bill.billID)
        {
            throw invalid_argument("Every bill line must have the ID of its bill.");
        }
    }

    billRepository.saveBill(bill);
    detailRepository.saveLines(bill.lines);
}

bool BillService::findBill(const string& billID, Bill& bill) const
{
    vector<Bill> bills = billRepository.getAllBills();

    for (int i = 0; i < (int)bills.size(); i++)
    {
        if (bills[i].billID == billID)
        {
            bill = bills[i];
            bill.lines = detailRepository.getLinesByBillID(billID);
            return true;
        }
    }

    return false;
}

string BillService::getCurrentDate()
{
    time_t now = time(0);
    char buffer[16];

    strftime(buffer, sizeof(buffer), "%d-%m-%Y", localtime(&now));
    return buffer;
}

string BillService::getCurrentTime()
{
    time_t now = time(0);
    char buffer[16];

    strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&now));
    return buffer;
}