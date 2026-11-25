#pragma once
#ifndef MANAGERSCREEN_H
#define MANAGERSCREEN_H

#include <QWidget>
#include <QFrame>
#include <QString>
#include <QImage>
#include "../Backend/Category/CategoryService.h"
#include "../Backend/Billing/SeasonRepository.h"

class ProductService;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QTableWidget;
class QGroupBox;
class QPropertyAnimation;
class QEvent;
class QEnterEvent;
class QResizeEvent;

class CatalogDrawer : public QFrame
{
    Q_OBJECT

public:
    explicit CatalogDrawer(QWidget* parent = nullptr);

    QWidget* contentContainer() const { return m_bodyWidget; }
    bool isPinned() const;
    void setExpanded(bool expanded, bool animated = true);
    void repositionInParent();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    static constexpr int kHandleWidth = 42;
    static constexpr int kPanelWidth = 740;

    QLabel* m_handleLabel = nullptr;
    QWidget* m_bodyWidget = nullptr;
    QCheckBox* m_pinCheck = nullptr;
    QPropertyAnimation* m_slideAnim = nullptr;
    bool m_isExpanded = false;
};

class ManagerScreen : public QWidget
{
    Q_OBJECT

public:
    explicit ManagerScreen(ProductService* productService,
                           const QString& productFilePath,
                           QWidget* parent = nullptr);

public slots:
    void refreshProductTable();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // Category Management
    void onAddCategoryClicked();
    void refreshCategoryList();
    void onCategorySelectionChanged(int index);

    // Product Management
    void onGenerateBarcodeClicked();
    void onSaveBarcodeImageClicked();
    void onSaveProductClicked();
    void onLoadSelectedForEditClicked();
    void onCancelEditClicked();
    void onRestockClicked();
    void onRemoveProductClicked();

    // Seasonal Discounts & Bill PDF Viewer
    void refreshSeasonsTable();
    void onAddSeasonClicked();
    void onToggleSeasonActiveClicked();
    void onOpenSavedBillsClicked();

private:
    void setupUi();
    void setStatusMessage(const QString& text, bool isError = false);
    QString selectedCatalogBarcode() const;
    QString selectedCatalogProductID() const;
    void resetProductForm();

    ProductService* m_productService;
    QString m_productFilePath;
    CategoryService m_categoryService;
    FileSeasonRepository m_seasonRepository;
    bool m_isEditingProduct = false;

    // Category Configuration Controls
    QLineEdit* m_catIdInput = nullptr;
    QLineEdit* m_catNameInput = nullptr;
    QLineEdit* m_catAcronymInput = nullptr;
    QPushButton* m_addCategoryBtn = nullptr;
    QTableWidget* m_categoryTable = nullptr;

    // Product Form Controls
    QGroupBox* m_formGroup = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QLineEdit* m_idInput = nullptr;
    QLineEdit* m_barcodePrefixLabel = nullptr;
    QLineEdit* m_barcodeSuffixInput = nullptr;
    QLineEdit* m_acronymInput = nullptr;
    QLineEdit* m_nameInput = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QDoubleSpinBox* m_priceSpin = nullptr;
    QDoubleSpinBox* m_taxSpin = nullptr;
    QDoubleSpinBox* m_discountSpin = nullptr;
    QSpinBox* m_quantitySpin = nullptr;
    QLineEdit* m_imagePathInput = nullptr;

    QPushButton* m_genBarcodeBtn = nullptr;
    QPushButton* m_saveBarcodeBtn = nullptr;
    QLabel* m_barcodePreview = nullptr;

    QPushButton* m_saveProductBtn = nullptr;
    QPushButton* m_cancelEditBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Hover-Expandable Catalog Drawer & Controls
    CatalogDrawer* m_catalogDrawer = nullptr;
    QTableWidget* m_productTable = nullptr;
    QSpinBox* m_restockSpin = nullptr;
    QPushButton* m_editProductBtn = nullptr;
    QPushButton* m_restockBtn = nullptr;
    QPushButton* m_removeProductBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;

    // Seasonal Discount & Saved Bills Controls
    QLineEdit* m_seasonStartInput = nullptr;
    QLineEdit* m_seasonEndInput = nullptr;
    QDoubleSpinBox* m_seasonPercentSpin = nullptr;
    QCheckBox* m_seasonActiveCheck = nullptr;
    QPushButton* m_addSeasonBtn = nullptr;
    QPushButton* m_toggleSeasonBtn = nullptr;
    QPushButton* m_viewBillsBtn = nullptr;
    QTableWidget* m_seasonsTable = nullptr;

    QImage m_generatedBarcodeImage;
};

#endif