#include "BillRepo3.h"
#include "RecordFormat3.h"
#include <fstream>
#include <stdexcept>
using namespace std;

int BillFileParser::fieldCount = 14;
int BillDetailFileParser::fieldCount = 5;

string BillFileParser::format(const Bill& bill)
{
    vector<string> fields;

    fields.push_back(RecordFormat::escapeText(bill.billID));
    fields.push_back(RecordFormat::escapeText(bill.customerName));
    fields.push_back(RecordFormat::escapeText(bill.orderID));
    fields.push_back(RecordFormat::formatNumber(bill.totalWithoutTax));
    fields.push_back(RecordFormat::formatNumber(bill.totalTax));
    fields.push_back(RecordFormat::formatNumber(bill.totalDiscount));
    fields.push_back(RecordFormat::formatNumber(bill.totalPayable));
    fields.push_back(RecordFormat::formatNumber(bill.amountReceived));
    fields.push_back(RecordFormat::formatNumber(bill.balanceReturned));
    fields.push_back(RecordFormat::escapeText(bill.paymentStatus));
    fields.push_back(RecordFormat::formatNumber(bill.seasonalDiscount));
    fields.push_back(RecordFormat::formatNumber(bill.specialDiscount));
    fields.push_back(RecordFormat::escapeText(bill.billDate));
    fields.push_back(RecordFormat::escapeText(bill.billTime));

    return RecordFormat::joinFields(fields);
}

bool BillFileParser::parse(const string& line, Bill& bill)
{
    vector<string> fields;

    if (!RecordFormat::splitLine(line, fields))
    {
        return false;
    }

    if ((int)fields.size() != fieldCount)
    {
        return false;
    }

    Bill result;
    result.billID = RecordFormat::unescapeText(fields[0]);
    result.customerName = RecordFormat::unescapeText(fields[1]);
    result.orderID = RecordFormat::unescapeText(fields[2]);
    result.paymentStatus = RecordFormat::unescapeText(fields[9]);
    result.billDate = RecordFormat::unescapeText(fields[12]);
    result.billTime = RecordFormat::unescapeText(fields[13]);

    if (!RecordFormat::toDouble(fields[3], result.totalWithoutTax) ||
        !RecordFormat::toDouble(fields[4], result.totalTax) ||
        !RecordFormat::toDouble(fields[5], result.totalDiscount) ||
        !RecordFormat::toDouble(fields[6], result.totalPayable) ||
        !RecordFormat::toDouble(fields[7], result.amountReceived) ||
        !RecordFormat::toDouble(fields[8], result.balanceReturned) ||
        !RecordFormat::toDouble(fields[10], result.seasonalDiscount) ||
        !RecordFormat::toDouble(fields[11], result.specialDiscount))
    {
        return false;
    }

    bill = result;
    return true;
}

string BillDetailFileParser::format(const BillLine& line)
{
    vector<string> fields;

    fields.push_back(RecordFormat::escapeText(line.billID));
    fields.push_back(RecordFormat::escapeText(line.productAcronym));
    fields.push_back(RecordFormat::formatNumber(line.unitPrice));
    fields.push_back(to_string(line.quantity));
    fields.push_back(RecordFormat::formatNumber(line.discount));

    return RecordFormat::joinFields(fields);
}

bool BillDetailFileParser::parse(const string& text, BillLine& line)
{
    vector<string> fields;

    if (!RecordFormat::splitLine(text, fields))
    {
        return false;
    }

    if ((int)fields.size() != fieldCount)
    {
        return false;
    }

    BillLine result;
    result.billID = RecordFormat::unescapeText(fields[0]);
    result.productAcronym = RecordFormat::unescapeText(fields[1]);

    if (!RecordFormat::toDouble(fields[2], result.unitPrice) ||
        !RecordFormat::toInt(fields[3], result.quantity) ||
        !RecordFormat::toDouble(fields[4], result.discount))
    {
        return false;
    }

    line = result;
    return true;
}

FileBillRepository::FileBillRepository(const string& filePath)
{
    this->filePath = filePath;
}

vector<Bill> FileBillRepository::getAllBills() const
{
    vector<Bill> bills;
    ifstream file(filePath.c_str());

    if (!file.is_open())
    {
        return bills;
    }

    string line;

    while (getline(file, line))
    {
        Bill bill;

        if (BillFileParser::parse(line, bill))
        {
            bills.push_back(bill);
        }
    }

    return bills;
}

void FileBillRepository::saveBill(const Bill& bill)
{
    vector<Bill> existing = getAllBills();

    for (int i = 0; i < (int)existing.size(); i++)
    {
        if (existing[i].billID == bill.billID)
        {
            throw invalid_argument("A bill with this ID already exists: " + bill.billID);
        }
    }

    ofstream file(filePath.c_str(), ios::app);

    if (!file.is_open())
    {
        throw runtime_error("Could not open the bill file for writing: " + filePath);
    }

    file << BillFileParser::format(bill) << endl;

    if (!file.good())
    {
        throw runtime_error("Could not write the bill file: " + filePath);
    }
}

FileBillDetailRepository::FileBillDetailRepository(const string& filePath)
{
    this->filePath = filePath;
}

vector<BillLine> FileBillDetailRepository::getLinesByBillID(const string& billID) const
{
    vector<BillLine> lines;
    ifstream file(filePath.c_str());

    if (!file.is_open())
    {
        return lines;
    }

    string text;

    while (getline(file, text))
    {
        BillLine line;

        if (BillDetailFileParser::parse(text, line) && line.billID == billID)
        {
            lines.push_back(line);
        }
    }

    return lines;
}

void FileBillDetailRepository::saveLines(const vector<BillLine>& lines)
{
    ofstream file(filePath.c_str(), ios::app);

    if (!file.is_open())
    {
        throw runtime_error("Could not open the bill detail file for writing: " + filePath);
    }

    for (int i = 0; i < (int)lines.size(); i++)
    {
        file << BillDetailFileParser::format(lines[i]) << endl;
    }

    if (!file.good())
    {
        throw runtime_error("Could not write the bill detail file: " + filePath);
    }
}