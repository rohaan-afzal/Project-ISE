#ifndef BILLREPO_H
#define BILLREPO_H

#include <string>
#include <vector>
#include "Bill3.h"
using namespace std;

class BillFileParser
{
private:
    static int fieldCount;

public:
    static string format(const Bill& bill);
    static bool parse(const string& line, Bill& bill);
};

class BillDetailFileParser
{
private:
    static int fieldCount;

public:
    static string format(const BillLine& line);
    static bool parse(const string& text, BillLine& line);
};

class BillRepository
{
public:
    virtual ~BillRepository() {}

    virtual vector<Bill> getAllBills() const = 0;
    virtual void saveBill(const Bill& bill) = 0;
};

class BillDetailRepository
{
public:
    virtual ~BillDetailRepository() {}

    virtual vector<BillLine> getLinesByBillID(const string& billID) const = 0;
    virtual void saveLines(const vector<BillLine>& lines) = 0;
};

class FileBillRepository : public BillRepository
{
private:
    string filePath;

public:
    explicit FileBillRepository(const string& filePath = "Data/Bill.txt");

    vector<Bill> getAllBills() const;
    void saveBill(const Bill& bill);
};

class FileBillDetailRepository : public BillDetailRepository
{
private:
    string filePath;

public:
    explicit FileBillDetailRepository(const string& filePath = "Data/BillDetail.txt");

    vector<BillLine> getLinesByBillID(const string& billID) const;
    void saveLines(const vector<BillLine>& lines);
};

#endif