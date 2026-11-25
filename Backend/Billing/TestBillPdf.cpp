#include "PdfBillGenerator3.h"
#include <QApplication>
#include <QFile>
#include <iostream>
#include <cstdio>
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

Bill makeBill()
{
    Bill bill;
    bill.billID = "000000001";
    bill.customerName = "Ali Khan";
    bill.orderID = "12345626";
    bill.paymentStatus = "paid";
    bill.billDate = "26-11-2026";
    bill.billTime = "14:30:00";

    BillLine milk;
    milk.billID = "000000001";
    milk.productAcronym = "MILK";
    milk.unitPrice = 220.0;
    milk.quantity = 2;
    milk.discount = 10.0;
    bill.lines.push_back(milk);

    BillLine rice;
    rice.billID = "000000001";
    rice.productAcronym = "RICE";
    rice.unitPrice = 380.0;
    rice.quantity = 1;
    bill.lines.push_back(rice);

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

    PdfBillGenerator generator;
    Bill bill = makeBill();
    QString html = generator.buildHtml(bill);

    report("text shows the bill ID", html.contains("000000001"));
    report("text shows the customer", html.contains("Ali Khan"));
    report("text shows date and time", html.contains("26-11-2026") && html.contains("14:30:00"));
    report("text shows every product", html.contains("MILK") && html.contains("RICE"));
    report("text shows the line amount", html.contains("PKR 430.00") && html.contains("PKR 380.00"));
    report("text shows total without tax", html.contains("PKR 820.00"));
    report("text shows total tax", html.contains("PKR 41.00"));
    report("text shows the seasonal discount", html.contains("PKR 85.10"));
    report("text shows the special discount", html.contains("PKR 42.55"));
    report("text shows the payable amount saved in Bill.txt", html.contains("PKR 723.35"));
    report("text shows amount received and balance", html.contains("PKR 1000.00") && html.contains("PKR 276.65"));

    Bill tricky = makeBill();
    tricky.customerName = "A <b>& B";
    report("special characters in names are escaped", generator.buildHtml(tricky).contains("A &lt;b&gt;&amp; B"));

    QString path = "test_bill_tmp.pdf";
    QFile::remove(path);
    report("PDF is generated", generator.generate(bill, path));

    QFile file(path);
    bool isPdf = file.open(QIODevice::ReadOnly) && file.size() > 500 && file.read(4) == "%PDF";
    file.close();
    report("PDF file is a real, non-empty PDF", isPdf);
    QFile::remove(path);

    report("PDF cannot be written to an impossible path", !generator.generate(bill, "no_such_folder/bill.pdf"));

    cout << endl << "Total: " << totalTests << "  Failed: " << failedTests << endl;

    if (failedTests == 0)
    {
        return 0;
    }
    return 1;
}