#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>
#include "Product.h"
using namespace std;

struct BillLine{
    string productAcronym;
    double unitPrice;
    double discount;
    double tax;
    int quantity;

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
    string paymentStatus;

    Bill();
};
#endif