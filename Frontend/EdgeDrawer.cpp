#include "EdgeDrawer.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QEnterEvent>

EdgeDrawer::EdgeDrawer(Side side,
    const QString& spineText,
    const QString& headerTitle,
    int panelWidth,
    QWidget* parent)
    : QFrame(parent),
    m_side(side),
    m_spineText(spineText.toUpper()),
    m_panelWidth(panelWidth)
{
    setObjectName("EdgeDrawer");

    if (m_side == Side::Right)
    {
        setStyleSheet(R"(
            QFrame#EdgeDrawer {
                background-color: #FFFFFF;
                border: 1.5px solid #D9785A;
                border-right: none;
                border-top-left-radius: 14px;
                border-bottom-left-radius: 14px;
            }
            QLabel#DrawerSpine {
                background-color: #D9785A;
                color: #FFFFFF;
                font-weight: bold;
                font-size: 12.5px;
                border-top-left-radius: 11px;
                border-bottom-left-radius: 11px;
                padding: 4px;
            }
        )");
    }
    else
    {
        setStyleSheet(R"(
            QFrame#EdgeDrawer {
                background-color: #FFFFFF;
                border: 1.5px solid #D9785A;
                border-left: none;
                border-top-right-radius: 14px;
                border-bottom-right-radius: 14px;
            }
            QLabel#DrawerSpine {
                background-color: #D9785A;
                color: #FFFFFF;
                font-weight: bold;
                font-size: 12.5px;
                border-top-right-radius: 11px;
                border-bottom-right-radius: 11px;
                padding: 4px;
            }
        )");
    }

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_spineLabel = new QLabel(formattedSpineText(false), this);
    m_spineLabel->setObjectName("DrawerSpine");
    m_spineLabel->setFixedWidth(kSpineWidth);
    m_spineLabel->setAlignment(Qt::AlignCenter);
    m_spineLabel->setCursor(Qt::PointingHandCursor);

    m_bodyWidget = new QWidget(this);
    m_bodyWidget->setFixedWidth(m_panelWidth);
    QVBoxLayout* bodyLayout = new QVBoxLayout(m_bodyWidget);
    bodyLayout->setContentsMargins(16, 14, 16, 14);
    bodyLayout->setSpacing(10);

    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(headerTitle, m_bodyWidget);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #3B2F2A;");

    m_pinCheck = new QCheckBox("Pin Open", m_bodyWidget);
    m_pinCheck->setStyleSheet("color: #5B8F94; font-weight: 600;");
    headerRow->addWidget(titleLabel);
    headerRow->addStretch();
    headerRow->addWidget(m_pinCheck);
    bodyLayout->addLayout(headerRow);

    if (m_side == Side::Right)
    {
        outer->addWidget(m_spineLabel);
        outer->addWidget(m_bodyWidget);
    }
    else
    {
        outer->addWidget(m_bodyWidget);
        outer->addWidget(m_spineLabel);
    }

    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(200);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_pinCheck, &QCheckBox::toggled, this, [this](bool pinned) {
        if (!pinned && !underMouse())
            setExpanded(false, true);
        });
}

QString EdgeDrawer::formattedSpineText(bool expanded) const
{
    QString arrow;
    if (m_side == Side::Right)
        arrow = expanded ? "?" : "?";
    else
        arrow = expanded ? "?" : "?";

    QString out = arrow + "\n";
    for (QChar ch : m_spineText)
    {
        out += "\n" + QString(ch);
    }
    return out;
}

bool EdgeDrawer::isPinned() const
{
    return m_pinCheck && m_pinCheck->isChecked();
}

void EdgeDrawer::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    setExpanded(true, true);
}

void EdgeDrawer::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    if (!isPinned())
        setExpanded(false, true);
}

void EdgeDrawer::repositionInParent()
{
    if (!parentWidget())
        return;

    const int pw = parentWidget()->width();
    const int ph = parentWidget()->height();
    const int drawerH = qMax(360, ph - 24);
    const int topY = (ph - drawerH) / 2;
    const int fullW = qMin(m_panelWidth + kSpineWidth, pw - 24);
    const int bodyW = fullW - kSpineWidth;

    m_bodyWidget->setFixedWidth(bodyW);
    setFixedSize(fullW, drawerH);

    int targetX = 0;
    if (m_side == Side::Right)
        targetX = m_isExpanded ? (pw - fullW) : (pw - kSpineWidth);
    else
        targetX = m_isExpanded ? 0 : -(fullW - kSpineWidth);

    m_spineLabel->setText(formattedSpineText(m_isExpanded));
    move(targetX, topY);
    raise();
}

void EdgeDrawer::setExpanded(bool expanded, bool animated)
{
    if (!parentWidget() || m_isExpanded == expanded)
        return;

    m_isExpanded = expanded;
    const int pw = parentWidget()->width();
    const int ph = parentWidget()->height();
    const int drawerH = qMax(360, ph - 24);
    const int topY = (ph - drawerH) / 2;
    const int fullW = qMin(m_panelWidth + kSpineWidth, pw - 24);

    int targetX = 0;
    if (m_side == Side::Right)
        targetX = expanded ? (pw - fullW) : (pw - kSpineWidth);
    else
        targetX = expanded ? 0 : -(fullW - kSpineWidth);

    m_spineLabel->setText(formattedSpineText(expanded));
    raise();

    if (!animated)
    {
        move(targetX, topY);
        return;
    }

    m_slideAnim->stop();
    m_slideAnim->setStartValue(pos());
    m_slideAnim->setEndValue(QPoint(targetX, topY));
    m_slideAnim->start();
}