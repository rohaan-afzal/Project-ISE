#include "BillRepo.h"
#include "RecordFormat.h"
#include <fstream>
#include <stdexcept>
using namespace std;

int BillFileParser::fieldCount = 12;
string BillFileParser::formatLine(const Bill& bill, const BillLine& line){
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

    for (int i = 0; i < (int)bill.lines.size(); i++){
        lines.push_back(formatLine(bill, bill.lines[i]));
    }

    return lines;
}

bool BillFileParser::parseLine(const string& line, Bill& bill){
    vector<string> fields;

    if (!RecordFormat::splitLine(line, fields)){
        return false;
    }

    if ((int)fields.size() != fieldCount){
        return false;
    }

    Bill result;
    BillLine billLine;

    result.billID = RecordFormat::unescapeText(fields[0]);
    result.customerName = RecordFormat::unescapeText(fields[1]);
    result.orderID = RecordFormat::unescapeText(fields[2]);
    billLine.productAcronym = RecordFormat::unescapeText(fields[3]);
    result.paymentStatus = RecordFormat::unescapeText(fields[11]);

    if (!RecordFormat::toDouble(fields[4], billLine.unitPrice) ||
        !RecordFormat::toDouble(fields[5], billLine.discount) ||
        !RecordFormat::toInt(fields[6], billLine.quantity) ||
        !RecordFormat::toDouble(fields[7], result.totalWithoutTax) ||
        !RecordFormat::toDouble(fields[8], result.totalTax) ||
        !RecordFormat::toDouble(fields[9], result.totalDiscount) ||
        !RecordFormat::toDouble(fields[10], result.totalPayable))
    {
        return false;
    }

    result.lines.push_back(billLine);
    bill = result;
    return true;
}

int BillFileParser::findBillIndex(const vector<Bill>& bills, const string& billID)
{
    for (int i = 0; i < (int)bills.size(); i++){
        if (bills[i].billID == billID){
            return i;
        }
    }

    return -1;
}

vector<Bill> BillFileParser::parseAll(const vector<string>& lines){
    vector<Bill> bills;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        Bill parsed;

        if (!parseLine(lines[i], parsed)){
            continue;
        }

        int index = findBillIndex(bills, parsed.billID);

        if (index < 0){
            bills.push_back(parsed);
        }
        else{
            bills[index].lines.push_back(parsed.lines[0]);
        }
    }

    return bills;
}

FileBillRepository::FileBillRepository(const string& filePath){
    this->filePath = filePath;
}

vector<Bill> FileBillRepository::getAllBills() const{
    vector<string> lines;
    ifstream file(filePath.c_str());

    if (!file.is_open()){
        return vector<Bill>();
    }

    string line;

    while (getline(file, line)){
        lines.push_back(line);
    }

    return BillFileParser::parseAll(lines);
}

void FileBillRepository::saveBill(const Bill& bill){
    ofstream file(filePath.c_str(), ios::app);

    if (!file.is_open()){
        throw runtime_error("Could not open the bill file for writing: " + filePath);
    }

    vector<string> lines = BillFileParser::format(bill);

    for (int i = 0; i < (int)lines.size(); i++){
        file << lines[i] << endl;
    }

    if (!file.good()){
        throw runtime_error("Could not write the bill file: " + filePath);
    }
}