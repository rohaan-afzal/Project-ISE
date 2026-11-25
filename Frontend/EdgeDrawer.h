#pragma once
#ifndef EDGEDRAWER_H
#define EDGEDRAWER_H

#include <QFrame>
#include <QString>

class QLabel;
class QWidget;
class QCheckBox;
class QPropertyAnimation;
class QEnterEvent;
class QEvent;

class EdgeDrawer : public QFrame
{
    Q_OBJECT

public:
    enum class Side
    {
        Left,
        Right
    };

    explicit EdgeDrawer(Side side,
        const QString& spineText,
        const QString& headerTitle,
        int panelWidth = 540,
        QWidget* parent = nullptr);

    QWidget* bodyWidget() const { return m_bodyWidget; }
    bool isPinned() const;
    bool isExpanded() const { return m_isExpanded; }

    void setExpanded(bool expanded, bool animated = true);
    void repositionInParent();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString formattedSpineText(bool expanded) const;

    static constexpr int kSpineWidth = 40;
    Side m_side;
    QString m_spineText;
    int m_panelWidth;

    QLabel* m_spineLabel = nullptr;
    QWidget* m_bodyWidget = nullptr;
    QCheckBox* m_pinCheck = nullptr;
    QPropertyAnimation* m_slideAnim = nullptr;
    bool m_isExpanded = false;
};

#endif