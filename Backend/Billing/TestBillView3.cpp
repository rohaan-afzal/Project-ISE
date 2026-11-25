#include "BillView3.h"
#include <QApplication>
#include <QPixmap>
#include <iostream>
using namespace std;

int totalTests = 0;
int failedTests = 0;

void report(string name, bool passed)
{
    totalTests++;
    if (!passed)
    {
        failedTests++;
    }

    if (passed)
    {
        cout << "[PASS] " << name << endl;
    }
    else
    {
        cout << "[FAIL] " << name << endl;
    }
}

BillLine makeLine(string billID, string acronym, double price, int quantity, double totalDiscount)
{
    BillLine line;
    line.billID = billID;
    line.productAcronym = acronym;
    line.unitPrice = price;
    line.quantity = quantity;
    line.discount = totalDiscount;
    return line;
}

Bill makeBill()
{
    Bill bill;
    bill.billID = "000000001";
    bill.customerName = "Ali Khan";
    bill.orderID = "12345626";
    bill.paymentStatus = "paid";
    bill.billDate = "26-11-2026";
    bill.billTime = "14:30:00";
    bill.lines.push_back(makeLine("000000001", "MILK", 220.0, 2, 10.0));
    bill.lines.push_back(makeLine("000000001", "RICE", 380.0, 1, 0.0));
    bill.totalWithoutTax = 820.0;
    bill.totalTax = 41.0;
    bill.totalDiscount = 10.0;
    bill.seasonalDiscount = 85.10;
    bill.specialDiscount = 42.55;
    bill.totalPayable = 723.35;
    bill.amountReceived = 1000.0;
    bill.balanceReturned = 276.65;
    return bill;
}

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    BillView view;
    view.resize(560, 560);
    report("empty view shows a dash", view.findChild<QLabel*>("billIDLabel")->text() == "-");

    view.showBill(makeBill());
    QTableWidget* table = view.findChild<QTableWidget*>("billTable");

    report("bill ID is shown", view.findChild<QLabel*>("billIDLabel")->text() == "000000001");
    report("customer is shown", view.findChild<QLabel*>("customerLabel")->text() == "Ali Khan");
    report("order ID is shown", view.findChild<QLabel*>("orderIDLabel")->text() == "12345626");
    report("status is shown", view.findChild<QLabel*>("statusLabel")->text() == "paid");
    report("date and time are shown", view.findChild<QLabel*>("dateTimeLabel")->text() == "26-11-2026 14:30:00");
    report("one row for each product", table->rowCount() == 2);
    report("product acronym is shown", table->item(0, 0)->text() == "MILK");
    report("quantity is shown", table->item(0, 2)->text() == "2");
    report("line discount is shown", table->item(0, 3)->text() == "PKR 10.00");
    report("line amount comes from PricingService", table->item(0, 4)->text() == "PKR 430.00");
    report("total without tax is shown", view.findChild<QLabel*>("totalWithoutTaxLabel")->text() == "PKR 820.00");
    report("total tax is shown", view.findChild<QLabel*>("totalTaxLabel")->text() == "PKR 41.00");
    report("product discounts are shown", view.findChild<QLabel*>("totalDiscountLabel")->text() == "PKR 10.00");
    report("seasonal discount is shown", view.findChild<QLabel*>("seasonalDiscountLabel")->text() == "PKR 85.10");
    report("special discount is shown", view.findChild<QLabel*>("specialDiscountLabel")->text() == "PKR 42.55");
    report("total payable is shown", view.findChild<QLabel*>("totalPayableLabel")->text() == "PKR 723.35");
    report("amount received is shown", view.findChild<QLabel*>("amountReceivedLabel")->text() == "PKR 1000.00");
    report("balance returned is shown", view.findChild<QLabel*>("balanceReturnedLabel")->text() == "PKR 276.65");

    view.grab().save("bill_view_preview.png");

    view.showBill(makeBill());
    report("showing a bill again does not repeat rows", table->rowCount() == 2);

    view.clear();
    report("clear removes the rows", table->rowCount() == 0);
    report("clear resets the totals", view.findChild<QLabel*>("totalPayableLabel")->text() == "-");

    cout << endl << "Total: " << totalTests << "  Failed: " << failedTests << endl;

    if (failedTests == 0)
    {
        return 0;
    }
    return 1;
}