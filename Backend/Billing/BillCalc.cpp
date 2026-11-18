#include "BillCalc.h"
#include "RecordFormat.h"
#include <cmath>
#include <stdexcept>
using namespace std;

double PricingService::roundMoney(double value){
    return floor(value * 100.0 + 0.5) / 100.0;
}

double PricingService::getLineSubtotal(const BillLine& line){
    return roundMoney(line.unitPrice * line.quantity);
}

double PricingService::getLineTax(const BillLine& line){
    return roundMoney(line.tax * line.quantity);
}

double PricingService::getLineDiscount(const BillLine& line){
    return roundMoney(line.discount * line.quantity);
}

double PricingService::getLineTotal(const BillLine& line){
    return roundMoney(getLineSubtotal(line) + getLineTax(line) - getLineDiscount(line));
}

double PricingService::getTotalWithoutTax(const vector<BillLine>& lines){
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineSubtotal(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getTotalTax(const vector<BillLine>& lines){
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineTax(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getTotalDiscount(const vector<BillLine>& lines){
    double total = 0.0;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        total += getLineDiscount(lines[i]);
    }

    return roundMoney(total);
}

double PricingService::getTotalPayable(const vector<BillLine>& lines){
    return roundMoney(getTotalWithoutTax(lines) + getTotalTax(lines) - getTotalDiscount(lines));
}

static int billIDLength = 9;
static const string validStatuses[4] = { "in-progress", "on-hold", "paid", "archived" };

BillService::BillService(BillRepository& repository) : repository(repository){}

bool BillService::isValidStatus(const string& status){
    for (int i = 0; i < 4; i++){
        if (validStatuses[i] == status){
            return true;
        }
    }

    return false;
}

void BillService::validateInput(const string& customerName, const string& orderID,
    const string& paymentStatus, const vector<BillLine>& lines){
    if (customerName.empty()){
        throw invalid_argument("Customer name is required.");
    }

    if (orderID.empty()){
        throw invalid_argument("Order ID is required.");
    }

    if (!isValidStatus(paymentStatus)){
        throw invalid_argument("Payment status must be in-progress, on-hold, paid or archived.");
    }

    if (lines.empty()){
        throw invalid_argument("A bill needs at least one product.");
    }

    for (int i = 0; i < (int)lines.size(); i++){
        const BillLine& line = lines[i];

        if (line.productAcronym.empty()){
            throw invalid_argument("Every bill line needs a product acronym.");
        }

        if (line.quantity <= 0){
            throw invalid_argument("Quantity must be at least 1 for " + line.productAcronym + ".");
        }

        if (line.unitPrice < 0 || line.tax < 0){
            throw invalid_argument("Price and tax cannot be negative for " + line.productAcronym + ".");
        }

        if (line.discount < 0 || line.discount > line.unitPrice){
            throw invalid_argument("Discount must be between 0 and the unit price for " + line.productAcronym + ".");
        }
    }
}

string BillService::getNextBillID() const{
    vector<Bill> bills = repository.getAllBills();
    int highest = 0;

    for (int i = 0; i < (int)bills.size(); i++){
        int id;

        if (RecordFormat::toInt(bills[i].billID, id) && id > highest){
            highest = id;
        }
    }

    if (highest >= 999999999){
        throw runtime_error("The bill ID limit has been reached.");
    }

    string nextID = to_string(highest + 1);

    while ((int)nextID.length() < billIDLength){
        nextID = "0" + nextID;
    }

    return nextID;
}

Bill BillService::createBill(const string& customerName, const string& orderID,
    const string& paymentStatus, const vector<BillLine>& lines) const
{
    validateInput(customerName, orderID, paymentStatus, lines);

    Bill bill;
    bill.billID = getNextBillID();
    bill.customerName = customerName;
    bill.orderID = orderID;
    bill.paymentStatus = paymentStatus;
    bill.lines = lines;
    bill.totalWithoutTax = PricingService::getTotalWithoutTax(lines);
    bill.totalTax = PricingService::getTotalTax(lines);
    bill.totalDiscount = PricingService::getTotalDiscount(lines);
    bill.totalPayable = PricingService::getTotalPayable(lines);

    return bill;
}
    
void BillService::saveBill(const Bill& bill){
    repository.saveBill(bill);
}