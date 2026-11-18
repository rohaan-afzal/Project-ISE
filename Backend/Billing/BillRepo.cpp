#include "BillRepo.h"
#include "RecordFormat.h"
#include <fstream>
#include <stdexcept>
using namespace std;

int BillFileParser::fieldCount = 12;
string BillFileParser::formatLine(const Bill& bill, const BillLine& line)
{
    vector<string> fields;

    fields.push_back(RecordFormat::escapeText(bill.billID));
    fields.push_back(RecordFormat::escapeText(bill.customerName));
    fields.push_back(RecordFormat::escapeText(bill.orderID));
    fields.push_back(RecordFormat::escapeText(line.productAcronym));
    fields.push_back(RecordFormat::formatNumber(line.unitPrice));
    fields.push_back(RecordFormat::formatNumber(line.discount));
    fields.push_back(to_string(line.quantity));
    fields.push_back(RecordFormat::formatNumber(bill.totalWithoutTax));
    fields.push_back(RecordFormat::formatNumber(bill.totalTax));
    fields.push_back(RecordFormat::formatNumber(bill.totalDiscount));
    fields.push_back(RecordFormat::formatNumber(bill.totalPayable));
    fields.push_back(RecordFormat::escapeText(bill.paymentStatus));

    return RecordFormat::joinFields(fields);
}

vector<string> BillFileParser::format(const Bill& bill){
    vector<string> lines;

    for (int i = 0; i < (int)bill.lines.size(); i++)
    {
        lines.push_back(formatLine(bill, bill.lines[i]));
    }

    return lines;
}