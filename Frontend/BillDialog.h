#pragma once
#ifndef BILLDIALOG_H
#define BILLDIALOG_H

#include <QDialog>
#include "../Backend/Billing/Bill.h"
#include "../Backend/Order/Payment.h"

class BillView;
class QLabel;

class BillDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BillDialog(const Bill& bill,
        const Payment& payment = Payment(),
        double amountTendered = 0.0,
        QWidget* parent = nullptr);

private:
    BillView* m_billView;
    QLabel* m_paymentSummaryLabel;
};

#endif