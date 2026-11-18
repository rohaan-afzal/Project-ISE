#include "Bill.h"
#include "BillCalc.h"
#include "BillRepo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <stdexcept>
using namespace std;

int totalTests = 0;
int failedTests = 0;

class InMemoryBillRepository : public BillRepository
{
private:
    vector<Bill> bills;

public:
    vector<Bill> getAllBills() const
    {
        return bills;
    }

    void saveBill(const Bill& bill)
    {
        bills.push_back(bill);
    }
};

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

bool isAbout(double actual, double expected)
{
    return fabs(actual - expected) < 0.0001;
}

BillLine makeLine(string acronym, double price, double discount, double tax, int quantity)
{
    BillLine line;
    line.productAcronym = acronym;
    line.unitPrice = price;
    line.discount = discount;
    line.tax = tax;
    line.quantity = quantity;
    return line;
}

vector<BillLine> makeTwoLines()
{
    vector<BillLine> lines;
    lines.push_back(makeLine("MILK", 220.0, 5.0, 11.0, 2));
    lines.push_back(makeLine("RICE", 380.0, 0.0, 19.0, 1));
    return lines;
}

bool createThrows(BillService& service, string customer, string orderID, string status, vector<BillLine> lines)
{
    try
    {
        service.createBill(customer, orderID, status, lines);
    }
    catch (invalid_argument&)
    {
        return true;
    }
    return false;
}

void testPricing()
{
    BillLine milk = makeLine("MILK", 220.0, 5.0, 11.0, 2);

    report("line subtotal is price x quantity", isAbout(PricingService::getLineSubtotal(milk), 440.0));
    report("line tax is tax x quantity", isAbout(PricingService::getLineTax(milk), 22.0));
    report("line discount is discount x quantity", isAbout(PricingService::getLineDiscount(milk), 10.0));
    report("line total is subtotal + tax - discount", isAbout(PricingService::getLineTotal(milk), 452.0));

    vector<BillLine> lines = makeTwoLines();
    report("total without tax", isAbout(PricingService::getTotalWithoutTax(lines), 820.0));
    report("total tax", isAbout(PricingService::getTotalTax(lines), 41.0));
    report("total discount", isAbout(PricingService::getTotalDiscount(lines), 10.0));
    report("total payable", isAbout(PricingService::getTotalPayable(lines), 851.0));

    vector<BillLine> empty;
    report("empty bill totals are 0", isAbout(PricingService::getTotalPayable(empty), 0.0));

    vector<BillLine> small;
    small.push_back(makeLine("A", 0.1, 0.0, 0.0, 1));
    small.push_back(makeLine("B", 0.2, 0.0, 0.0, 1));
    report("0.1 + 0.2 gives exactly 0.30", PricingService::getTotalWithoutTax(small) == 0.30);

    BillLine free = makeLine("FREE", 0.0, 0.0, 0.0, 5);
    report("free product costs nothing", isAbout(PricingService::getLineTotal(free), 0.0));

    BillLine product = makeLine("RICE", 380.0, 19.0, 19.0, 3);
    report("full discount line is correct", isAbout(PricingService::getLineTotal(product), 1140.0));
}

void testBillLineFromProduct()
{
    Product product;
    product.acronym = "MILK";
    product.unitPrice = 220.0;
    product.discount = 5.0;
    product.tax = 11.0;

    BillLine line(product, 3);
    report("bill line copies product values", line.productAcronym == "MILK" && line.unitPrice == 220.0
        && line.discount == 5.0 && line.tax == 11.0 && line.quantity == 3);
}

Bill makeBill()
{
    InMemoryBillRepository repository;
    BillService service(repository);
    return service.createBill("Ali Khan", "12345617", "paid", makeTwoLines());
}

void testParser()
{
    Bill bill = makeBill();
    vector<string> lines = BillFileParser::format(bill);

    report("one text line for each product", lines.size() == 2);
    report("first line is in the specified order",
        lines[0] == "000000001 Ali%20Khan 12345617 MILK 220.00 5.00 2 820.00 41.00 10.00 851.00 paid #");
    report("second line is in the specified order",
        lines[1] == "000000001 Ali%20Khan 12345617 RICE 380.00 0.00 1 820.00 41.00 10.00 851.00 paid #");

    vector<Bill> parsed = BillFileParser::parseAll(lines);
    report("lines with the same bill ID become one bill", parsed.size() == 1 && parsed[0].lines.size() == 2);
    report("bill details are read back",
        parsed[0].billID == "000000001" && parsed[0].customerName == "Ali Khan"
        && parsed[0].orderID == "12345617" && parsed[0].paymentStatus == "paid");
    report("bill totals are read back",
        isAbout(parsed[0].totalWithoutTax, 820.0) && isAbout(parsed[0].totalTax, 41.0)
        && isAbout(parsed[0].totalDiscount, 10.0) && isAbout(parsed[0].totalPayable, 851.0));
    report("product lines are read back",
        parsed[0].lines[1].productAcronym == "RICE" && isAbout(parsed[0].lines[1].unitPrice, 380.0)
        && parsed[0].lines[1].quantity == 1);

    vector<string> mixed;
    mixed.push_back(lines[0]);
    mixed.push_back("not a bill line");
    mixed.push_back("000000002 Sara 12345618 SOAP 120.00 0.00 1 120.00 6.00 0.00 126.00 on-hold");
    mixed.push_back("000000002 Sara 12345618 SOAP abc 0.00 1 120.00 6.00 0.00 126.00 on-hold #");
    mixed.push_back("000000003 Sara 12345618 SOAP 120.00 0.00 1 120.00 6.00 0.00 126.00 on-hold #");
    parsed = BillFileParser::parseAll(mixed);
    report("bad lines are skipped", parsed.size() == 2 && parsed[1].billID == "000000003");

    report("empty input gives no bills", BillFileParser::parseAll(vector<string>()).size() == 0);
}

void testRepository()
{
    string path = "test_bills_tmp.txt";
    remove(path.c_str());

    FileBillRepository repository(path);
    report("missing file gives no bills", repository.getAllBills().size() == 0);

    Bill bill = makeBill();
    repository.saveBill(bill);

    Bill second = bill;
    second.billID = "000000002";
    second.customerName = "Sara";
    repository.saveBill(second);

    vector<Bill> loaded = repository.getAllBills();
    report("saved bills are loaded back", loaded.size() == 2 && loaded[1].customerName == "Sara");
    report("first bill keeps both products", loaded[0].lines.size() == 2);

    ifstream file(path.c_str());
    string line;
    int count = 0;
    bool allEndWithMarker = true;
    while (getline(file, line))
    {
        count++;
        if (line.substr(line.length() - 2) != " #")
        {
            allEndWithMarker = false;
        }
    }
    file.close();
    report("file has one line per product, each ending with #", count == 4 && allEndWithMarker);

    FileBillRepository badRepository("no_such_folder/bills.txt");
    bool threw = false;
    try
    {
        badRepository.saveBill(bill);
    }
    catch (runtime_error&)
    {
        threw = true;
    }
    report("saving to an impossible path throws runtime_error", threw);

    remove(path.c_str());
}

void testService()
{
    InMemoryBillRepository repository;
    BillService service(repository);

    Bill first = service.createBill("Ali Khan", "12345617", "paid", makeTwoLines());
    report("first bill ID is 000000001", first.billID == "000000001");
    report("bill ID has 9 digits", first.billID.length() == 9);
    report("bill totals are calculated",
        isAbout(first.totalWithoutTax, 820.0) && isAbout(first.totalTax, 41.0)
        && isAbout(first.totalDiscount, 10.0) && isAbout(first.totalPayable, 851.0));
    report("bill keeps customer, order and status",
        first.customerName == "Ali Khan" && first.orderID == "12345617" && first.paymentStatus == "paid");

    report("creating a bill does not save it", repository.getAllBills().size() == 0);
    service.saveBill(first);
    report("saving stores the bill", repository.getAllBills().size() == 1);

    Bill second = service.createBill("Sara", "12345618", "in-progress", makeTwoLines());
    report("next bill ID is 000000002", second.billID == "000000002");

    Bill existing;
    existing.billID = "000000099";
    repository.saveBill(existing);
    Bill third = service.createBill("Sara", "12345619", "on-hold", makeTwoLines());
    report("next ID follows the highest saved ID", third.billID == "000000100");

    vector<BillLine> lines = makeTwoLines();
    report("empty customer name is rejected", createThrows(service, "", "1", "paid", lines));
    report("empty order ID is rejected", createThrows(service, "Ali", "", "paid", lines));
    report("unknown status is rejected", createThrows(service, "Ali", "1", "finished", lines));
    report("status archived is accepted", !createThrows(service, "Ali", "1", "archived", lines));
    report("status in-progress is accepted", !createThrows(service, "Ali", "1", "in-progress", lines));
    report("bill with no products is rejected", createThrows(service, "Ali", "1", "paid", vector<BillLine>()));

    vector<BillLine> bad = makeTwoLines();
    bad[0].quantity = 0;
    report("quantity 0 is rejected", createThrows(service, "Ali", "1", "paid", bad));

    bad = makeTwoLines();
    bad[0].unitPrice = -5;
    report("negative price is rejected", createThrows(service, "Ali", "1", "paid", bad));

    bad = makeTwoLines();
    bad[1].discount = 500;
    report("discount above price is rejected", createThrows(service, "Ali", "1", "paid", bad));

    bad = makeTwoLines();
    bad[0].productAcronym = "";
    report("empty acronym is rejected", createThrows(service, "Ali", "1", "paid", bad));
}

int main(){
    testPricing();
    testBillLineFromProduct();
    testParser();
    testRepository();
    testService();

    cout << endl << "Total: " << totalTests << "  Failed: " << failedTests << endl;

    if (failedTests == 0)
    {
        return 0;
    }
    return 1;
}