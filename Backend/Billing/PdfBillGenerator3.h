#ifndef PDFBILLGENERATOR_H
#define PDFBILLGENERATOR_H

#include <QString>
#include "Bill3.h"

class PdfBillGenerator
{
private:
    static QString formatMoney(double value);
    static QString makeRow(const QString& label, double value);

public:
    QString buildHtml(const Bill& bill) const;
    bool generate(const Bill& bill, const QString& filePath) const;
};

#endif