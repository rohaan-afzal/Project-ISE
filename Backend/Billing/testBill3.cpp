#include "Bill3.h"
#include "BillCalc3.h"
#include "BillRepo3.h"
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
        for (int i = 0; i < (int)bills.size(); i++)
        {
            if (bills[i].billID == bill.billID)
            {
                throw invalid_argument("A bill with this ID already exists.");
            }
        }
        bills.push_back(bill);
    }
};

class InMemoryBillDetailRepository : public BillDetailRepository
{
public:
    vector<BillLine> allLines;

    vector<BillLine> getLinesByBillID(const string& billID) const
    {
        vector<BillLine> result;
        for (int i = 0; i < (int)allLines.size(); i++)
        {
            if (allLines[i].billID == billID)
            {
                result.push_back(allLines[i]);
            }
        }
        return result;
    }

    void saveLines(const vector<BillLine>& lines)
    {
        for (int i = 0; i < (int)lines.size(); i++)
        {
            allLines.push_back(lines[i]);
        }
    }
};

class InMemorySeasonSource : public SeasonSource
{
public:
    vector<SeasonRule> rules;

    vector<SeasonRule> getSeasonRules() const
    {
        return rules;
    }

    void add(string start, string end, double percentage, bool isActive)
    {
        SeasonRule rule;
        rule.startDate = start;
        rule.endDate = end;
        rule.percentage = percentage;
        rule.isActive = isActive;
        rules.push_back(rule);
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

BillLine makeLine(string acronym, double price, int quantity, double totalDiscount, double unitTax)
{
    BillLine line;
    line.productAcronym = acronym;
    line.unitPrice = price;
    line.quantity = quantity;
    line.discount = totalDiscount;
    line.unitTax = unitTax;
    return line;
}

vector<BillLine> makeTwoLines()
{
    vector<BillLine> lines;
    lines.push_back(makeLine("MILK", 220.0, 2, 10.0, 11.0));
    lines.push_back(makeLine("RICE", 380.0, 1, 0.0, 19.0));
    return lines;
}

BillRequest makeRequest(string status, double received, double specialPercent)
{
    BillRequest request;
    request.customerName = "Ali Khan";
    request.orderID = "12345626";
    request.paymentStatus = status;
    request.lines = makeTwoLines();
    request.amountReceived = received;
    request.specialDiscountPercent = specialPercent;
    request.billDate = "26-11-2026";
    request.billTime = "14:30:00";
    return request;
}

bool createThrows(const BillService& service, const BillRequest& request)
{
    try
    {
        service.createBill(request);
    }
    catch (invalid_argument&)
    {
        return true;
    }
    return false;
}

void testPricing()
{
    BillLine milk = makeLine("MILK", 220.0, 2, 10.0, 11.0);

    report("line subtotal is price x quantity", isAbout(PricingService::getLineSubtotal(milk), 440.0));
    report("line tax is unit tax x quantity", isAbout(PricingService::getLineTax(milk), 22.0));
    report("line discount is the stored total", isAbout(PricingService::getLineDiscount(milk), 10.0));
    report("line amount is subtotal - discount", isAbout(PricingService::getLineAmount(milk), 430.0));

    vector<BillLine> lines = makeTwoLines();
    report("total without tax", isAbout(PricingService::getTotalWithoutTax(lines), 820.0));
    report("total tax", isAbout(PricingService::getTotalTax(lines), 41.0));
    report("total product discount", isAbout(PricingService::getTotalDiscount(lines), 10.0));
    report("amount after product discount", isAbout(PricingService::getAmountAfterProductDiscount(lines), 851.0));

    report("10% of 851 is 85.10", isAbout(PricingService::getPercentAmount(851.0, 10.0), 85.10));
    report("5% of 851 is 42.55", isAbout(PricingService::getPercentAmount(851.0, 5.0), 42.55));
    report("0% gives 0", isAbout(PricingService::getPercentAmount(851.0, 0.0), 0.0));
    report("total payable after both discounts", isAbout(PricingService::getTotalPayable(851.0, 85.10, 42.55), 723.35));

    report("balance returned is received - payable", isAbout(PricingService::getBalanceReturned(1000.0, 723.35), 276.65));
    report("exact payment gives no balance", isAbout(PricingService::getBalanceReturned(723.35, 723.35), 0.0));
    report("balance is never negative", isAbout(PricingService::getBalanceReturned(500.0, 723.35), 0.0));

    vector<BillLine> empty;
    report("empty bill totals are 0", isAbout(PricingService::getAmountAfterProductDiscount(empty), 0.0));

    vector<BillLine> small;
    small.push_back(makeLine("A", 0.1, 1, 0.0, 0.0));
    small.push_back(makeLine("B", 0.2, 1, 0.0, 0.0));
    report("0.1 + 0.2 gives exactly 0.30", PricingService::getTotalWithoutTax(small) == 0.30);

    Product product;
    product.acronym = "MILK";
    product.unitPrice = 220.0;
    product.discount = 5.0;
    product.tax = 11.0;
    BillLine fromProduct(product, 10);
    report("bill line from product: discount is for the whole quantity", isAbout(fromProduct.discount, 50.0));
    report("bill line from product copies the other values",
        fromProduct.productAcronym == "MILK" && fromProduct.unitPrice == 220.0 && fromProduct.quantity == 10 && fromProduct.unitTax == 11.0);
}

void testSeasons()
{
    InMemorySeasonSource source;
    source.add("20-11-2026", "30-11-2026", 10.0, true);
    source.add("01-12-2026", "31-12-2026", 25.0, true);
    source.add("01-06-2026", "30-06-2026", 50.0, false);
    source.add("25-11-2026", "05-12-2026", 15.0, true);
    SeasonalDiscountService service(source);

    report("date inside one season", isAbout(service.getDiscountPercent("21-11-2026"), 10.0));
    report("overlapping seasons use the highest percentage", isAbout(service.getDiscountPercent("26-11-2026"), 15.0));
    report("start date is included", isAbout(service.getDiscountPercent("20-11-2026"), 10.0));
    report("end date is included", isAbout(service.getDiscountPercent("31-12-2026"), 25.0));
    report("day before a season gives 0", isAbout(service.getDiscountPercent("19-11-2026"), 0.0));
    report("day after all seasons gives 0", isAbout(service.getDiscountPercent("01-01-2027"), 0.0));
    report("inactive season is ignored", isAbout(service.getDiscountPercent("15-06-2026"), 0.0));

    bool threw = false;
    try
    {
        service.getDiscountPercent("31-02-2026");
    }
    catch (invalid_argument&)
    {
        threw = true;
    }
    report("invalid bill date throws", threw);

    InMemorySeasonSource badSource;
    badSource.add("bad", "bad", 10.0, true);
    badSource.add("01-01-2026", "31-12-2026", 150.0, true);
    SeasonalDiscountService badService(badSource);
    report("seasons with bad dates or percentage are ignored", isAbout(badService.getDiscountPercent("26-11-2026"), 0.0));

    InMemorySeasonSource noSeasons;
    SeasonalDiscountService emptyService(noSeasons);
    report("no seasons gives 0", isAbout(emptyService.getDiscountPercent("26-11-2026"), 0.0));
}

void testDateAndTimeChecks()
{
    report("valid date", SeasonalDiscountService::isValidDate("26-11-2026"));
    report("29 February in a leap year is valid", SeasonalDiscountService::isValidDate("29-02-2028"));
    report("29 February in a normal year is invalid", !SeasonalDiscountService::isValidDate("29-02-2026"));
    report("31 April is invalid", !SeasonalDiscountService::isValidDate("31-04-2026"));
    report("day 0 is invalid", !SeasonalDiscountService::isValidDate("00-01-2026"));
    report("month 13 is invalid", !SeasonalDiscountService::isValidDate("10-13-2026"));
    report("date without leading zeros is invalid", !SeasonalDiscountService::isValidDate("5-1-2026"));
    report("year-first date is invalid", !SeasonalDiscountService::isValidDate("2026-11-26"));
    report("empty date is invalid", !SeasonalDiscountService::isValidDate(""));

    report("valid time", SeasonalDiscountService::isValidTime("14:30:00"));
    report("24:00:00 is invalid", !SeasonalDiscountService::isValidTime("24:00:00"));
    report("60 minutes is invalid", !SeasonalDiscountService::isValidTime("12:60:00"));
    report("time without seconds is invalid", !SeasonalDiscountService::isValidTime("12:30"));
    report("time without leading zero is invalid", !SeasonalDiscountService::isValidTime("9:30:00"));

    report("current date is valid", SeasonalDiscountService::isValidDate(BillService::getCurrentDate()));
    report("current time is valid", SeasonalDiscountService::isValidTime(BillService::getCurrentTime()));
}

Bill makeSavedBill()
{
    InMemoryBillRepository billRepository;
    InMemoryBillDetailRepository detailRepository;
    InMemorySeasonSource source;
    source.add("20-11-2026", "30-11-2026", 10.0, true);
    SeasonalDiscountService seasons(source);
    BillService service(billRepository, detailRepository, seasons);
    return service.createBill(makeRequest("paid", 1000.0, 5.0));
}

void testParsers()
{
    Bill bill = makeSavedBill();
    string line = BillFileParser::format(bill);

    report("Bill.txt line has the fields in the specified order",
        line == "000000001 Ali%20Khan 12345626 820.00 41.00 10.00 723.35 1000.00 276.65 paid 85.10 42.55 26-11-2026 14:30:00 #");

    Bill parsed;
    report("Bill.txt line is read back", BillFileParser::parse(line, parsed));
    report("bill details are read back",
        parsed.billID == "000000001" && parsed.customerName == "Ali Khan" && parsed.orderID == "12345626"
        && parsed.paymentStatus == "paid" && parsed.billDate == "26-11-2026" && parsed.billTime == "14:30:00");
    report("bill amounts are read back",
        isAbout(parsed.totalWithoutTax, 820.0) && isAbout(parsed.totalTax, 41.0) && isAbout(parsed.totalDiscount, 10.0)
        && isAbout(parsed.totalPayable, 723.35) && isAbout(parsed.amountReceived, 1000.0)
        && isAbout(parsed.balanceReturned, 276.65) && isAbout(parsed.seasonalDiscount, 85.10)
        && isAbout(parsed.specialDiscount, 42.55));

    Bill other;
    report("bill line without # is rejected", !BillFileParser::parse("000000001 Ali 1 820.00 41.00 10.00 723.35 1000.00 276.65 paid 85.10 42.55 26-11-2026 14:30:00", other));
    report("bill line with 13 fields is rejected", !BillFileParser::parse("000000001 Ali 1 820.00 41.00 10.00 723.35 1000.00 276.65 paid 85.10 42.55 26-11-2026 #", other));
    report("bill line with a text amount is rejected", !BillFileParser::parse("000000001 Ali 1 abc 41.00 10.00 723.35 1000.00 276.65 paid 85.10 42.55 26-11-2026 14:30:00 #", other));
    report("empty bill line is rejected", !BillFileParser::parse("", other));
    report("failed parse leaves the bill unchanged", other.billID == "");

    report("BillDetail.txt line for MILK", BillDetailFileParser::format(bill.lines[0]) == "000000001 MILK 220.00 2 10.00 #");
    report("BillDetail.txt line for RICE", BillDetailFileParser::format(bill.lines[1]) == "000000001 RICE 380.00 1 0.00 #");

    BillLine parsedLine;
    report("BillDetail.txt line is read back", BillDetailFileParser::parse("000000001 MILK 220.00 2 10.00 #", parsedLine));
    report("bill line values are read back",
        parsedLine.billID == "000000001" && parsedLine.productAcronym == "MILK" && isAbout(parsedLine.unitPrice, 220.0)
        && parsedLine.quantity == 2 && isAbout(parsedLine.discount, 10.0));

    BillLine otherLine;
    report("detail line with 4 fields is rejected", !BillDetailFileParser::parse("000000001 MILK 220.00 2 #", otherLine));
    report("detail line with a decimal quantity is rejected", !BillDetailFileParser::parse("000000001 MILK 220.00 2.5 10.00 #", otherLine));
    report("detail line without # is rejected", !BillDetailFileParser::parse("000000001 MILK 220.00 2 10.00", otherLine));
}

void testRepositories()
{
    string billPath = "test_bills_tmp.txt";
    string detailPath = "test_bill_details_tmp.txt";
    remove(billPath.c_str());
    remove(detailPath.c_str());

    FileBillRepository billRepository(billPath);
    FileBillDetailRepository detailRepository(detailPath);

    report("missing bill file gives no bills", billRepository.getAllBills().size() == 0);
    report("missing detail file gives no lines", detailRepository.getLinesByBillID("000000001").size() == 0);

    Bill first = makeSavedBill();
    Bill second = first;
    second.billID = "000000002";
    second.customerName = "Sara";
    for (int i = 0; i < (int)second.lines.size(); i++)
    {
        second.lines[i].billID = "000000002";
    }

    billRepository.saveBill(first);
    billRepository.saveBill(second);
    detailRepository.saveLines(first.lines);
    detailRepository.saveLines(second.lines);

    vector<Bill> bills = billRepository.getAllBills();
    report("saved bills are loaded back", bills.size() == 2 && bills[1].customerName == "Sara");

    bool threw = false;
    try
    {
        billRepository.saveBill(first);
    }
    catch (invalid_argument&)
    {
        threw = true;
    }
    report("a bill ID cannot be saved twice", threw && billRepository.getAllBills().size() == 2);

    vector<BillLine> lines = detailRepository.getLinesByBillID("000000002");
    report("lines of one bill are found", lines.size() == 2 && lines[0].productAcronym == "MILK" && lines[0].billID == "000000002");
    report("unknown bill ID has no lines", detailRepository.getLinesByBillID("000000009").size() == 0);

    ofstream billFile(billPath.c_str(), ios::app);
    billFile << "this is not a bill" << endl;
    billFile.close();
    ofstream detailFile(detailPath.c_str(), ios::app);
    detailFile << "this is not a detail" << endl;
    detailFile.close();

    report("bad lines are skipped in Bill.txt", billRepository.getAllBills().size() == 2);
    report("bad lines are skipped in BillDetail.txt", detailRepository.getLinesByBillID("000000001").size() == 2);

    ifstream file(billPath.c_str());
    string text;
    int count = 0;
    bool allEndWithMarker = true;
    while (getline(file, text))
    {
        if (text.find("not a bill") != string::npos)
        {
            continue;
        }
        count++;
        if (text.substr(text.length() - 2) != " #")
        {
            allEndWithMarker = false;
        }
    }
    file.close();
    report("Bill.txt has one line per bill, each ending with #", count == 2 && allEndWithMarker);

    FileBillRepository badBillRepository("no_such_folder/bills.txt");
    threw = false;
    try
    {
        badBillRepository.saveBill(first);
    }
    catch (runtime_error&)
    {
        threw = true;
    }
    report("saving a bill to an impossible path throws runtime_error", threw);

    FileBillDetailRepository badDetailRepository("no_such_folder/details.txt");
    threw = false;
    try
    {
        badDetailRepository.saveLines(first.lines);
    }
    catch (runtime_error&)
    {
        threw = true;
    }
    report("saving lines to an impossible path throws runtime_error", threw);

    remove(billPath.c_str());
    remove(detailPath.c_str());
}

void testServiceCalculation()
{
    InMemoryBillRepository billRepository;
    InMemoryBillDetailRepository detailRepository;
    InMemorySeasonSource source;
    SeasonalDiscountService seasons(source);
    BillService service(billRepository, detailRepository, seasons);

    Bill bill = service.createBill(makeRequest("paid", 1000.0, 0.0));
    report("first bill ID is 000000001", bill.billID == "000000001");
    report("bill ID has 9 digits", bill.billID.length() == 9);
    report("bill keeps customer, order, status, date and time",
        bill.customerName == "Ali Khan" && bill.orderID == "12345626" && bill.paymentStatus == "paid"
        && bill.billDate == "26-11-2026" && bill.billTime == "14:30:00");
    report("totals without any extra discount",
        isAbout(bill.totalWithoutTax, 820.0) && isAbout(bill.totalTax, 41.0) && isAbout(bill.totalDiscount, 10.0)
        && isAbout(bill.seasonalDiscount, 0.0) && isAbout(bill.specialDiscount, 0.0) && isAbout(bill.totalPayable, 851.0));
    report("amount received and balance", isAbout(bill.amountReceived, 1000.0) && isAbout(bill.balanceReturned, 149.0));
    report("every line gets the bill ID", bill.lines.size() == 2 && bill.lines[0].billID == bill.billID && bill.lines[1].billID == bill.billID);

    source.add("20-11-2026", "30-11-2026", 10.0, true);
    bill = service.createBill(makeRequest("paid", 1000.0, 5.0));
    report("seasonal discount comes from the active season", isAbout(bill.seasonalDiscount, 85.10));
    report("special discount is a percentage of the amount", isAbout(bill.specialDiscount, 42.55));
    report("payable after seasonal and special discount", isAbout(bill.totalPayable, 723.35));
    report("balance after discounts", isAbout(bill.balanceReturned, 276.65));

    BillRequest outsideSeason = makeRequest("paid", 1000.0, 0.0);
    outsideSeason.billDate = "10-12-2026";
    report("no seasonal discount outside the season", isAbout(service.createBill(outsideSeason).seasonalDiscount, 0.0));

    BillRequest onHold = makeRequest("on-hold", 0.0, 0.0);
    Bill held = service.createBill(onHold);
    report("an on-hold bill needs no payment", isAbout(held.amountReceived, 0.0) && isAbout(held.balanceReturned, 0.0));

    report("creating a bill does not save it", billRepository.getAllBills().size() == 0 && detailRepository.allLines.size() == 0);
}

void testServiceSaving()
{
    InMemoryBillRepository billRepository;
    InMemoryBillDetailRepository detailRepository;
    InMemorySeasonSource source;
    SeasonalDiscountService seasons(source);
    BillService service(billRepository, detailRepository, seasons);

    Bill first = service.createBill(makeRequest("paid", 1000.0, 0.0));
    service.saveBill(first);
    report("saving stores the bill", billRepository.getAllBills().size() == 1);
    report("saving stores every product line", detailRepository.allLines.size() == 2);

    Bill second = service.createBill(makeRequest("paid", 1000.0, 0.0));
    report("next bill ID is 000000002", second.billID == "000000002");

    bool threw = false;
    try
    {
        service.saveBill(first);
    }
    catch (invalid_argument&)
    {
        threw = true;
    }
    report("saving the same bill twice is rejected", threw);
    report("a rejected bill leaves no extra lines", detailRepository.allLines.size() == 2);

    Bill broken = second;
    broken.lines[1].billID = "000000555";
    threw = false;
    try
    {
        service.saveBill(broken);
    }
    catch (invalid_argument&)
    {
        threw = true;
    }
    report("a line with another bill ID is rejected", threw && billRepository.getAllBills().size() == 1);

    Bill found;
    report("a saved bill is found with its lines", service.findBill("000000001", found) && found.lines.size() == 2 && found.customerName == "Ali Khan");
    report("an unknown bill is not found", !service.findBill("000000777", found));

    Bill highest;
    highest.billID = "000000099";
    billRepository.saveBill(highest);
    report("next ID follows the highest saved ID", service.createBill(makeRequest("paid", 1000.0, 0.0)).billID == "000000100");
}

void testServiceValidation()
{
    InMemoryBillRepository billRepository;
    InMemoryBillDetailRepository detailRepository;
    InMemorySeasonSource source;
    SeasonalDiscountService seasons(source);
    BillService service(billRepository, detailRepository, seasons);

    BillRequest request = makeRequest("paid", 1000.0, 0.0);
    report("a correct request is accepted", !createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.customerName = "";
    report("empty customer name is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.orderID = "";
    report("empty order ID is rejected", createThrows(service, request));

    request = makeRequest("finished", 1000.0, 0.0);
    report("unknown status is rejected", createThrows(service, request));

    report("status archived is accepted", !createThrows(service, makeRequest("archived", 0.0, 0.0)));
    report("status in-progress is accepted", !createThrows(service, makeRequest("in-progress", 0.0, 0.0)));

    request = makeRequest("paid", 1000.0, 0.0);
    request.billDate = "31-02-2026";
    report("invalid bill date is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.billTime = "25:00:00";
    report("invalid bill time is rejected", createThrows(service, request));

    report("special discount of exactly 8% is accepted", !createThrows(service, makeRequest("paid", 1000.0, 8.0)));
    report("special discount above 8% is rejected", createThrows(service, makeRequest("paid", 1000.0, 8.01)));
    report("negative special discount is rejected", createThrows(service, makeRequest("paid", 1000.0, -1.0)));
    report("negative amount received is rejected", createThrows(service, makeRequest("paid", -5.0, 0.0)));
    report("paid bill with too little money is rejected", createThrows(service, makeRequest("paid", 500.0, 0.0)));
    report("paid bill with exact money is accepted", !createThrows(service, makeRequest("paid", 851.0, 0.0)));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines.clear();
    report("bill with no products is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines[0].quantity = 0;
    report("quantity 0 is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines[0].unitPrice = -5;
    report("negative price is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines[0].unitTax = -1;
    report("negative tax is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines[1].discount = 500;
    report("discount above the price of the products is rejected", createThrows(service, request));

    request = makeRequest("paid", 1000.0, 0.0);
    request.lines[0].productAcronym = "";
    report("empty acronym is rejected", createThrows(service, request));
}

void testWithFiles()
{
    string billPath = "test_bills_flow_tmp.txt";
    string detailPath = "test_bill_details_flow_tmp.txt";
    remove(billPath.c_str());
    remove(detailPath.c_str());

    InMemorySeasonSource source;
    source.add("20-11-2026", "30-11-2026", 10.0, true);
    SeasonalDiscountService seasons(source);

    FileBillRepository billRepository(billPath);
    FileBillDetailRepository detailRepository(detailPath);
    BillService service(billRepository, detailRepository, seasons);

    Bill bill = service.createBill(makeRequest("paid", 1000.0, 5.0));
    service.saveBill(bill);

    FileBillRepository secondBillRepository(billPath);
    FileBillDetailRepository secondDetailRepository(detailPath);
    BillService secondService(secondBillRepository, secondDetailRepository, seasons);

    Bill found;
    bool isFound = secondService.findBill("000000001", found);
    report("a bill saved in files is found again", isFound);
    report("saved bill keeps its totals", isAbout(found.totalPayable, 723.35) && isAbout(found.seasonalDiscount, 85.10) && isAbout(found.specialDiscount, 42.55));
    report("saved bill keeps its products", found.lines.size() == 2 && found.lines[0].productAcronym == "MILK" && isAbout(found.lines[0].discount, 10.0));
    report("next bill ID after saving is 000000002", secondService.createBill(makeRequest("paid", 1000.0, 0.0)).billID == "000000002");

    remove(billPath.c_str());
    remove(detailPath.c_str());
}

int main()
{
    testPricing();
    testSeasons();
    testDateAndTimeChecks();
    testParsers();
    testRepositories();
    testServiceCalculation();
    testServiceSaving();
    testServiceValidation();
    testWithFiles();

    cout << endl << "Total: " << totalTests << "  Failed: " << failedTests << endl;

    if (failedTests == 0)
    {
        return 0;
    }
    return 1;
}