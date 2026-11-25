#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>
#include "Product.h"
using namespace std;

struct BillLine{
    string billID;
    string productAcronym;
    double unitPrice;
    int quantity;
    double discount;
    double unitTax;

    BillLine();
    BillLine(const Product& product, int quantity);
};

struct Bill{
    string billID;
    string customerName;
    string orderID;
    vector<BillLine> lines;
    double totalWithoutTax;
    double totalTax;
    double totalDiscount;
    double totalPayable;
    double amountReceived;
    double balanceReturned;
    string paymentStatus;
    double seasonalDiscount;
    double specialDiscount;
    string billDate;
    string billTime;

    Bill();
};

struct BillRequest{
    string customerName;
    string orderID;
    string paymentStatus;
    vector<BillLine> lines;
    double amountReceived;
    double specialDiscountPercent;
    string billDate;
    string billTime;

    BillRequest();
};

#endif