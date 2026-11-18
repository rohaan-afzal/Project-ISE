#include "BillDialog.h"
#include "../Backend/Billing/BillView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <algorithm>

BillDialog::BillDialog(const Bill& bill,
    const Payment& payment,
    double amountTendered,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Customer Receipt - Bill #" + QString::fromStdString(bill.billID));
    resize(620, 520);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* titleLabel = new QLabel("<h2>POS Receipt</h2>", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    m_billView = new BillView(this);
    m_billView->showBill(bill);
    mainLayout->addWidget(m_billView, 1);

    QFrame* paymentBox = new QFrame(this);
    paymentBox->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout* paymentLayout = new QVBoxLayout(paymentBox);

    const double change = (payment.mode == "cash" && amountTendered > bill.totalPayable)
        ? std::max(0.0, amountTendered - bill.totalPayable)
        : 0.0;

    QString summary = QString("<b>Payment Mode:</b> %1 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Merchant:</b> %2")
        .arg(QString::fromStdString(payment.mode).toUpper(),
            QString::fromStdString(payment.merchant).toHtmlEscaped());

    if (payment.mode == "cash" && amountTendered > 0.0)
    {
        summary += QString("<br><b>Tendered:</b> PKR %1 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Change Returned:</b> PKR %2")
            .arg(QString::number(amountTendered, 'f', 2),
                QString::number(change, 'f', 2));
    }

    m_paymentSummaryLabel = new QLabel(summary, this);
    m_paymentSummaryLabel->setWordWrap(true);
    paymentLayout->addWidget(m_paymentSummaryLabel);
    mainLayout->addWidget(paymentBox);

    QHBoxLayout* footerRow = new QHBoxLayout();
    footerRow->addStretch();
    QPushButton* closeBtn = new QPushButton("Done / Next Order", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footerRow->addWidget(closeBtn);
    mainLayout->addLayout(footerRow);
}