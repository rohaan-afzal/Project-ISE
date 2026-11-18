#ifndef BILLREPO_H
#define BILLREPO_H

#include <string>
#include <vector>
#include "Bill.h"
using namespace std;

class BillFileParser{
private:
    static int fieldCount;

    static string formatLine(const Bill& bill, const BillLine& line);
    static bool parseLine(const string& line, Bill& bill);
    static int findBillIndex(const vector<Bill>& bills, const string& billID);

public:
    static vector<string> format(const Bill& bill);
    static vector<Bill> parseAll(const vector<string>& lines);
};

class BillRepository{
public:
    virtual ~BillRepository() {}

    virtual vector<Bill> getAllBills() const = 0;
    virtual void saveBill(const Bill& bill) = 0;
};

class FileBillRepository : public BillRepository{
private:
    string filePath;

public:
    explicit FileBillRepository(const string& filePath = "Bill.txt");

    vector<Bill> getAllBills() const;
    void saveBill(const Bill& bill);
};

#endif