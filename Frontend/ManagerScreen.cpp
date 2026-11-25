#include "ManagerScreen.h"
#include "BillDialog.h"
#include "../Backend/Product/ProductService.h"
#include "../Backend/Encoder/Encoder.h"
#include "../Backend/imageGenerator/BarcodeImageGenerator.h"
#include "../Backend/Billing/BillRepo3.h"
#include "../Backend/Billing/BillCalc3.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPixmap>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDialog>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QResizeEvent>
#include <stdexcept>
#include <vector>

namespace {
QString resolveSiblingDataPath(const QString& productFilePath, const QString& fileName)
{
    QFileInfo info(productFilePath);
    return info.dir().filePath(fileName);
}
}

// ============================================================================
// CatalogDrawer (Full-Height, Zero-Reflow Hover Right-Edge Sidebar)
// ============================================================================

CatalogDrawer::CatalogDrawer(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("CatalogDrawer");
    setStyleSheet(R"(
    /* ---------- Base ---------- */
    QWidget            { background-color:#FBF6EE; color:#3B2F2A; font-size:13px; }
    QLabel             { background:transparent; }

    /* ---------- Your existing drawer (unchanged) ---------- */
    QFrame#CatalogDrawer {
        background-color:#FFFFFF;
        border:2px solid #D97757;
        border-right:none;
        border-top-left-radius:12px;
        border-bottom-left-radius:12px;
    }
    QLabel#DrawerHandle {
        background-color:#D97757;
        color:#FFFFFF;
        font-weight:bold;
        font-size:13px;
        border-top-left-radius:9px;
        border-bottom-left-radius:9px;
        padding:4px;
    }

    /* ---------- Mirrored drawer for left-edge panels (e.g. SEASONS, TODAY) ---------- */
    QFrame#LeftDrawer {
        background-color:#FFFFFF;
        border:2px solid #D97757;
        border-left:none;
        border-top-right-radius:12px;
        border-bottom-right-radius:12px;
    }
    QLabel#LeftDrawerHandle {
        background-color:#D97757;
        color:#FFFFFF;
        font-weight:bold;
        font-size:13px;
        border-top-right-radius:9px;
        border-bottom-right-radius:9px;
        padding:4px;
    }

    /* ---------- Cards (group boxes) ---------- */
    QGroupBox {
        background:#FFFFFF;
        border:2px solid #F0CBBB;
        border-radius:12px;
        margin-top:16px;
        padding:12px 8px 8px 8px;
        font-weight:bold;
    }
    QGroupBox::title {
        subcontrol-origin:margin;
        left:14px;
        padding:2px 12px;
        background:#D97757;
        color:#FFFFFF;
        border-radius:8px;
    }

    /* ---------- Tabs as pills ---------- */
    QTabWidget::pane   { border:none; }
    QTabBar::tab {
        background:transparent;
        color:#3B2F2A;
        padding:8px 20px;
        margin:4px 3px;
        border-radius:14px;
    }
    QTabBar::tab:hover    { background:#FBE3D8; }
    QTabBar::tab:selected { background:#D97757; color:#FFFFFF; font-weight:bold; }

    /* ---------- Buttons ---------- */
    QPushButton {
        background:#D97757; color:#FFFFFF;
        border:2px solid #D97757; border-radius:10px;
        padding:8px 14px; font-weight:600;
    }
    QPushButton:hover    { background:#C4623F; border-color:#C4623F; }
    QPushButton:pressed  { background:#B35836; }
    QPushButton:disabled { background:#E8E1D8; border-color:#E8E1D8; color:#A89F96; }

    /* Secondary: setProperty("role","secondary") */
    QPushButton[role="secondary"] {
        background:#FFFFFF; color:#D97757; border:2px solid #D97757;
    }
    QPushButton[role="secondary"]:hover { background:#FBE3D8; }

    /* Destructive: setProperty("role","danger") */
    QPushButton[role="danger"] {
        background:#FFFFFF; color:#B5524A; border:2px solid #D9A39E;
    }
    QPushButton[role="danger"]:hover { background:#F9E3E0; }

    /* ---------- Inputs ---------- */
    QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
        background:#FFFFFF;
        border:1.5px solid #E6D8CC;
        border-radius:10px;
        padding:6px 10px;
        selection-background-color:#D97757;
    }
    QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
        border:2px solid #5B8F94;          /* calm teal focus ring */
    }
    QComboBox QAbstractItemView {
        background:#FFFFFF; border:1px solid #F0CBBB;
        selection-background-color:#FBE3D8; selection-color:#3B2F2A;
    }

    /* ---------- Tables ---------- */
    QTableView, QTableWidget {
        background:#FFFFFF;
        alternate-background-color:#FDF9F3;
        gridline-color:transparent;
        border:1.5px solid #F0CBBB;
        border-radius:12px;
        selection-background-color:#FBE3D8;
        selection-color:#3B2F2A;
    }
    QTableView::item, QTableWidget::item { padding:6px; border:none; }
    QHeaderView::section {
        background:#EFE6DA; color:#3B2F2A;
        border:none; padding:10px 6px; font-weight:bold;
    }

    /* ---------- Hero total (Cashier) ---------- */
    QLabel#HeroTotal {
        background:#D97757; color:#FFFFFF;
        border-radius:14px; padding:14px 20px;
        font-size:26px; font-weight:bold;
    }

    /* ---------- KPI chips (Reports) ---------- */
    QFrame#KpiCard {
        background:#FFFFFF; border:2px solid #F0CBBB; border-radius:12px;
    }
    QLabel#KpiLabel { color:#8A7A70; font-size:11px; }
    QLabel#KpiValue { color:#D97757; font-size:20px; font-weight:bold; }

    /* ---------- Status pills (setProperty("status", ...)) ---------- */
    QLabel[status="healthy"] { background:#E3EFE6; color:#4F7A5C; border-radius:10px; padding:2px 10px; }
    QLabel[status="low"]     { background:#FBEBCB; color:#9A6B12; border-radius:10px; padding:2px 10px; }
    QLabel[status="out"]     { background:#F6D9D5; color:#A8433B; border-radius:10px; padding:2px 10px; }

    /* ---------- Scrollbars ---------- */
    QScrollBar:vertical   { background:transparent; width:10px; margin:2px; }
    QScrollBar::handle:vertical { background:#E8B9A5; border-radius:4px; min-height:30px; }
    QScrollBar::handle:vertical:hover { background:#D97757; }
    QScrollBar::add-line, QScrollBar::sub-line { height:0; width:0; }
)");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Full-height vertical edge bar
    m_handleLabel = new QLabel("◀\n\nC\nA\nT\nA\nL\nO\nG", this);
    m_handleLabel->setObjectName("DrawerHandle");
    m_handleLabel->setFixedWidth(kHandleWidth);
    m_handleLabel->setAlignment(Qt::AlignCenter);
    m_handleLabel->setCursor(Qt::PointingHandCursor);
    m_handleLabel->setToolTip("Hover to open Product Catalog & Inventory");
    outer->addWidget(m_handleLabel);

    // Fixed width prevents QTableWidget from recalculating columns mid-slide
    m_bodyWidget = new QWidget(this);
    m_bodyWidget->setFixedWidth(kPanelWidth);
    QVBoxLayout* bodyLayout = new QVBoxLayout(m_bodyWidget);
    bodyLayout->setContentsMargins(14, 12, 14, 12);
    bodyLayout->setSpacing(8);

    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* title = new QLabel("Product Catalog & Inventory (Data/Product.txt)", m_bodyWidget);
    title->setStyleSheet("font-size: 15px; font-weight: bold; color: #141413;");
    m_pinCheck = new QCheckBox("Pin Open", m_bodyWidget);
    m_pinCheck->setToolTip("Keep catalog drawer open while editing or restocking");
    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(m_pinCheck);
    bodyLayout->addLayout(headerRow);

    outer->addWidget(m_bodyWidget);

    // Animate pos instead of geometry so zero layout reflows happen during slide
    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(85);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_pinCheck, &QCheckBox::toggled, this, [this](bool pinned) {
        if (!pinned && !underMouse())
            setExpanded(false, true);
    });
}

bool CatalogDrawer::isPinned() const
{
    return m_pinCheck && m_pinCheck->isChecked();
}

void CatalogDrawer::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    setExpanded(true, true);
}

void CatalogDrawer::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    if (!isPinned())
        setExpanded(false, true);
}

void CatalogDrawer::repositionInParent()
{
    if (!parentWidget())
        return;

    const int pw = parentWidget()->width();
    const int ph = parentWidget()->height();
    const int drawerH = qMax(360, ph - 24);
    const int topY = (ph - drawerH) / 2;
    const int fullW = qMin(kPanelWidth + kHandleWidth, pw - 20);
    const int bodyW = fullW - kHandleWidth;

    m_bodyWidget->setFixedWidth(bodyW);
    setFixedSize(fullW, drawerH);

    const int targetX = m_isExpanded ? (pw - fullW) : (pw - kHandleWidth);
    m_handleLabel->setText(m_isExpanded ? "▶\n\nC\nA\nT\nA\nL\nO\nG" : "◀\n\nC\nA\nT\nA\nL\nO\nG");
    move(targetX, topY);
    raise();
}

void CatalogDrawer::setExpanded(bool expanded, bool animated)
{
    if (!parentWidget() || m_isExpanded == expanded)
        return;

    m_isExpanded = expanded;
    const int pw = parentWidget()->width();
    const int ph = parentWidget()->height();
    const int drawerH = qMax(360, ph - 24);
    const int topY = (ph - drawerH) / 2;
    const int fullW = qMin(kPanelWidth + kHandleWidth, pw - 20);

    const int targetX = expanded ? (pw - fullW) : (pw - kHandleWidth);
    m_handleLabel->setText(expanded ? "▶\n\nC\nA\nT\nA\nL\nO\nG" : "◀\n\nC\nA\nT\nA\nL\nO\nG");
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

// ============================================================================
// ManagerScreen
// ============================================================================

ManagerScreen::ManagerScreen(ProductService* productService,
                             const QString& productFilePath,
                             QWidget* parent)
    : QWidget(parent),
    m_productService(productService),
    m_productFilePath(productFilePath),
    m_categoryService(resolveSiblingDataPath(productFilePath, "PCategory.txt").toStdString()),
    m_seasonRepository(resolveSiblingDataPath(productFilePath, "Seasons.txt").toStdString())
{
    setupUi();
    refreshCategoryList();
    refreshProductTable();
    refreshSeasonsTable();
}

void ManagerScreen::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_catalogDrawer)
        m_catalogDrawer->repositionInParent();
}

void ManagerScreen::setupUi()
{
    // Leave 56px right margin so the collapsed Catalog bar never covers widgets
    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 56, 12);
    rootLayout->setSpacing(16);

    // ================= LEFT COLUMN: Add / Edit Product & Barcode =================
    m_formGroup = new QGroupBox("1. Add New Product to Catalog", this);
    QVBoxLayout* formGroupLayout = new QVBoxLayout(m_formGroup);
    formGroupLayout->setSpacing(8);

    QFormLayout* form = new QFormLayout();
    form->setVerticalSpacing(7);
    form->setHorizontalSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumHeight(28);

    m_idInput = new QLineEdit(this);
    m_idInput->setMinimumHeight(28);
    m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));

    QHBoxLayout* barcodeFieldRow = new QHBoxLayout();
    barcodeFieldRow->setSpacing(6);
    m_barcodePrefixLabel = new QLineEdit(this);
    m_barcodePrefixLabel->setReadOnly(true);
    m_barcodePrefixLabel->setFixedWidth(50);
    m_barcodePrefixLabel->setMinimumHeight(28);
    m_barcodePrefixLabel->setAlignment(Qt::AlignCenter);
    m_barcodePrefixLabel->setStyleSheet("background-color: #E8E6DC; font-weight: bold;");

    m_barcodeSuffixInput = new QLineEdit(this);
    m_barcodeSuffixInput->setPlaceholderText("4 digits (e.g. 0001)");
    m_barcodeSuffixInput->setMaxLength(4);
    m_barcodeSuffixInput->setMinimumHeight(28);
    m_barcodeSuffixInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[0-9]{0,4}"), m_barcodeSuffixInput));

    barcodeFieldRow->addWidget(m_barcodePrefixLabel);
    barcodeFieldRow->addWidget(m_barcodeSuffixInput);

    m_acronymInput = new QLineEdit(this);
    m_acronymInput->setPlaceholderText("e.g. TEA");
    m_acronymInput->setMinimumHeight(28);

    m_nameInput = new QLineEdit(this);
    m_nameInput->setPlaceholderText("e.g. GreenTea");
    m_nameInput->setMinimumHeight(28);

    m_unitCombo = new QComboBox(this);
    m_unitCombo->setMinimumHeight(28);
    m_unitCombo->addItems(QStringList() << "Pack" << "Kg" << "Gram" << "Litre" << "ml" << "Piece" << "Box" << "Bottle" << "Dozen");

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.0, 1000000.0);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setPrefix("PKR ");
    m_priceSpin->setMinimumHeight(28);

    m_taxSpin = new QDoubleSpinBox(this);
    m_taxSpin->setRange(0.0, 100000.0);
    m_taxSpin->setDecimals(2);
    m_taxSpin->setMinimumHeight(28);

    m_discountSpin = new QDoubleSpinBox(this);
    m_discountSpin->setRange(0.0, 100000.0);
    m_discountSpin->setDecimals(2);
    m_discountSpin->setMinimumHeight(28);

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(0, 100000);
    m_quantitySpin->setValue(10);
    m_quantitySpin->setMinimumHeight(28);

    m_imagePathInput = new QLineEdit(this);
    m_imagePathInput->setPlaceholderText("Click Save Barcode PNG to record saved file path");
    m_imagePathInput->setMinimumHeight(28);

    form->addRow("Category:", m_categoryCombo);
    form->addRow("Product ID:", m_idInput);
    form->addRow("Barcode (Cat + 4 digits):", barcodeFieldRow);
    form->addRow("Acronym:", m_acronymInput);
    form->addRow("Name:", m_nameInput);
    form->addRow("Unit:", m_unitCombo);
    form->addRow("Unit Price:", m_priceSpin);
    form->addRow("Tax:", m_taxSpin);
    form->addRow("Discount:", m_discountSpin);
    form->addRow("Stock Quantity:", m_quantitySpin);
    form->addRow("Saved Image Path:", m_imagePathInput);
    formGroupLayout->addLayout(form);

    QHBoxLayout* barcodeBtns = new QHBoxLayout();
    m_genBarcodeBtn = new QPushButton("Preview Barcode", this);
    m_saveBarcodeBtn = new QPushButton("Save Barcode PNG", this);
    m_saveBarcodeBtn->setEnabled(false);
    barcodeBtns->addWidget(m_genBarcodeBtn);
    barcodeBtns->addWidget(m_saveBarcodeBtn);
    formGroupLayout->addLayout(barcodeBtns);

    m_barcodePreview = new QLabel("Barcode preview", this);
    m_barcodePreview->setAlignment(Qt::AlignCenter);
    m_barcodePreview->setMinimumHeight(70);
    m_barcodePreview->setMaximumHeight(85);
    m_barcodePreview->setFrameShape(QFrame::Box);
    formGroupLayout->addWidget(m_barcodePreview);

    QHBoxLayout* saveActionBtns = new QHBoxLayout();
    m_saveProductBtn = new QPushButton("Save Product to Catalog", this);
    m_cancelEditBtn = new QPushButton("Cancel Edit", this);
    m_cancelEditBtn->setVisible(false);
    saveActionBtns->addWidget(m_saveProductBtn);
    saveActionBtns->addWidget(m_cancelEditBtn);
    formGroupLayout->addLayout(saveActionBtns);

    m_statusLabel = new QLabel("Hover over the right-edge Catalog bar anytime to view, edit, or restock products.", this);
    m_statusLabel->setWordWrap(true);
    formGroupLayout->addWidget(m_statusLabel);
    formGroupLayout->addStretch();

    rootLayout->addWidget(m_formGroup, 5);

    // ================= RIGHT COLUMN: Categories + Seasonal Discounts =================
    QVBoxLayout* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);

    // 2. Category Configuration Group
    QGroupBox* catGroup = new QGroupBox("2. Product Categories (Data/PCategory.txt)", this);
    QVBoxLayout* catLayout = new QVBoxLayout(catGroup);
    catLayout->setSpacing(8);

    QHBoxLayout* catFormRow = new QHBoxLayout();
    m_catIdInput = new QLineEdit(this);
    m_catIdInput->setPlaceholderText("ID (10-99)");
    m_catIdInput->setMaxLength(2);
    m_catIdInput->setMaximumWidth(85);
    m_catIdInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]"), m_catIdInput));
    m_catIdInput->setText(QString::fromStdString(m_categoryService.getNextCategoryID()));

    m_catNameInput = new QLineEdit(this);
    m_catNameInput->setPlaceholderText("Category Name");

    m_catAcronymInput = new QLineEdit(this);
    m_catAcronymInput->setPlaceholderText("Acronym");
    m_catAcronymInput->setMaximumWidth(100);

    m_addCategoryBtn = new QPushButton("Add Category", this);

    catFormRow->addWidget(m_catIdInput);
    catFormRow->addWidget(m_catNameInput, 1);
    catFormRow->addWidget(m_catAcronymInput);
    catFormRow->addWidget(m_addCategoryBtn);
    catLayout->addLayout(catFormRow);

    m_categoryTable = new QTableWidget(0, 3, this);
    m_categoryTable->setHorizontalHeaderLabels(QStringList() << "Category ID" << "Category Name" << "Acronym");
    m_categoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_categoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryTable->verticalHeader()->setVisible(false);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    catLayout->addWidget(m_categoryTable, 1);

    rightColumn->addWidget(catGroup, 1);

    // 3. Seasonal Discounts & Saved Bills PDF Group
    QGroupBox* seasonsGroup = new QGroupBox("3. Seasonal Discounts (Data/Seasons.txt) && Saved Bills PDF", this);
    QVBoxLayout* seasonsLayout = new QVBoxLayout(seasonsGroup);
    seasonsLayout->setSpacing(8);

    QHBoxLayout* seasonFormRow = new QHBoxLayout();
    m_seasonStartInput = new QLineEdit(this);
    m_seasonStartInput->setPlaceholderText("Start DD-MM-YYYY");
    m_seasonStartInput->setMaxLength(10);

    m_seasonEndInput = new QLineEdit(this);
    m_seasonEndInput->setPlaceholderText("End DD-MM-YYYY");
    m_seasonEndInput->setMaxLength(10);

    m_seasonPercentSpin = new QDoubleSpinBox(this);
    m_seasonPercentSpin->setRange(0.0, 100.0);
    m_seasonPercentSpin->setDecimals(2);
    m_seasonPercentSpin->setSuffix(" %");
    m_seasonPercentSpin->setValue(10.0);

    m_seasonActiveCheck = new QCheckBox("Active", this);
    m_seasonActiveCheck->setChecked(true);

    m_addSeasonBtn = new QPushButton("Add Season", this);

    seasonFormRow->addWidget(m_seasonStartInput, 1);
    seasonFormRow->addWidget(m_seasonEndInput, 1);
    seasonFormRow->addWidget(m_seasonPercentSpin);
    seasonFormRow->addWidget(m_seasonActiveCheck);
    seasonFormRow->addWidget(m_addSeasonBtn);
    seasonsLayout->addLayout(seasonFormRow);

    m_seasonsTable = new QTableWidget(0, 4, this);
    m_seasonsTable->setHorizontalHeaderLabels(
        QStringList() << "Start Date" << "End Date" << "Discount (%)" << "Status");
    m_seasonsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_seasonsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_seasonsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_seasonsTable->verticalHeader()->setVisible(false);
    m_seasonsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    seasonsLayout->addWidget(m_seasonsTable, 1);

    QHBoxLayout* seasonActionsRow = new QHBoxLayout();
    m_toggleSeasonBtn = new QPushButton("Toggle Selected Active/Inactive", this);
    m_viewBillsBtn = new QPushButton("View / Print Saved Bills (PDF)", this);
    seasonActionsRow->addWidget(m_toggleSeasonBtn);
    seasonActionsRow->addStretch();
    seasonActionsRow->addWidget(m_viewBillsBtn);
    seasonsLayout->addLayout(seasonActionsRow);

    rightColumn->addWidget(seasonsGroup, 1);
    rootLayout->addLayout(rightColumn, 5);

    // ================= FLOATING RIGHT-EDGE CATALOG DRAWER =================
    m_catalogDrawer = new CatalogDrawer(this);
    QVBoxLayout* drawerBody = qobject_cast<QVBoxLayout*>(m_catalogDrawer->contentContainer()->layout());

    m_productTable = new QTableWidget(0, 10, m_catalogDrawer);
    m_productTable->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Cat ID" << "Barcode" << "Acronym" << "Name" << "Unit"
                      << "Price" << "Tax" << "Discount" << "Stock");
    m_productTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_productTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_productTable->verticalHeader()->setVisible(false);
    m_productTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    drawerBody->addWidget(m_productTable, 1);

    QHBoxLayout* stockActionsRow = new QHBoxLayout();
    m_editProductBtn = new QPushButton("Edit Selected", m_catalogDrawer);
    m_restockSpin = new QSpinBox(m_catalogDrawer);
    m_restockSpin->setRange(1, 100000);
    m_restockSpin->setValue(10);
    m_restockSpin->setPrefix("+ Stock: ");

    m_restockBtn = new QPushButton("Restock", m_catalogDrawer);
    m_removeProductBtn = new QPushButton("Remove", m_catalogDrawer);
    m_refreshBtn = new QPushButton("Refresh", m_catalogDrawer);

    stockActionsRow->addWidget(m_editProductBtn);
    stockActionsRow->addWidget(m_restockSpin);
    stockActionsRow->addWidget(m_restockBtn);
    stockActionsRow->addWidget(m_removeProductBtn);
    stockActionsRow->addWidget(m_refreshBtn);
    drawerBody->addLayout(stockActionsRow);

    m_catalogDrawer->repositionInParent();

    // Signal connections
    connect(m_addCategoryBtn, &QPushButton::clicked, this, &ManagerScreen::onAddCategoryClicked);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ManagerScreen::onCategorySelectionChanged);
    connect(m_genBarcodeBtn, &QPushButton::clicked, this, &ManagerScreen::onGenerateBarcodeClicked);
    connect(m_saveBarcodeBtn, &QPushButton::clicked, this, &ManagerScreen::onSaveBarcodeImageClicked);
    connect(m_saveProductBtn, &QPushButton::clicked, this, &ManagerScreen::onSaveProductClicked);
    connect(m_editProductBtn, &QPushButton::clicked, this, &ManagerScreen::onLoadSelectedForEditClicked);
    connect(m_cancelEditBtn, &QPushButton::clicked, this, &ManagerScreen::onCancelEditClicked);
    connect(m_restockBtn, &QPushButton::clicked, this, &ManagerScreen::onRestockClicked);
    connect(m_removeProductBtn, &QPushButton::clicked, this, &ManagerScreen::onRemoveProductClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ManagerScreen::refreshProductTable);

    connect(m_addSeasonBtn, &QPushButton::clicked, this, &ManagerScreen::onAddSeasonClicked);
    connect(m_toggleSeasonBtn, &QPushButton::clicked, this, &ManagerScreen::onToggleSeasonActiveClicked);
    connect(m_viewBillsBtn, &QPushButton::clicked, this, &ManagerScreen::onOpenSavedBillsClicked);
}

void ManagerScreen::setStatusMessage(const QString& text, bool isError)
{
    m_statusLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
    m_statusLabel->setText(text);
}

void ManagerScreen::refreshCategoryList()
{
    const std::vector<ProductCategory> categories = m_categoryService.getAllCategories();

    m_categoryTable->setRowCount(0);
    m_categoryCombo->clear();

    for (const auto& c : categories)
    {
        const int row = m_categoryTable->rowCount();
        m_categoryTable->insertRow(row);
        m_categoryTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.categoryID)));
        m_categoryTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c.categoryName)));
        m_categoryTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c.categoryAcronym)));

        const QString display = QString::fromStdString(c.categoryID + " - " + c.categoryName + " (" + c.categoryAcronym + ")");
        m_categoryCombo->addItem(display, QString::fromStdString(c.categoryID));
    }

    onCategorySelectionChanged(m_categoryCombo->currentIndex());
}

void ManagerScreen::onCategorySelectionChanged(int index)
{
    if (index < 0)
    {
        m_barcodePrefixLabel->clear();
        return;
    }
    m_barcodePrefixLabel->setText(m_categoryCombo->itemData(index).toString());
}

void ManagerScreen::onAddCategoryClicked()
{
    ProductCategory cat;
    cat.categoryID = m_catIdInput->text().trimmed().toStdString();
    cat.categoryName = m_catNameInput->text().trimmed().replace(' ', '_').toStdString();
    cat.categoryAcronym = m_catAcronymInput->text().trimmed().toUpper().toStdString();

    if (m_categoryService.addCategory(cat))
    {
        m_catNameInput->clear();
        m_catAcronymInput->clear();
        refreshCategoryList();
        m_catIdInput->setText(QString::fromStdString(m_categoryService.getNextCategoryID()));
        setStatusMessage("Added category " + QString::fromStdString(cat.categoryID + " - " + cat.categoryName) + ".");
    }
    else
    {
        setStatusMessage("Failed to add category. Use a unique 2-digit ID (10-99), Name, and Acronym.", true);
    }
}

QString ManagerScreen::selectedCatalogBarcode() const
{
    const int row = m_productTable->currentRow();
    if (row < 0 || !m_productTable->item(row, 2))
        return QString();
    return m_productTable->item(row, 2)->text();
}

QString ManagerScreen::selectedCatalogProductID() const
{
    const int row = m_productTable->currentRow();
    if (row < 0 || !m_productTable->item(row, 0))
        return QString();
    return m_productTable->item(row, 0)->text();
}

void ManagerScreen::onGenerateBarcodeClicked()
{
    const QString prefix = m_barcodePrefixLabel->text().trimmed();
    const QString suffix = m_barcodeSuffixInput->text().trimmed();

    if (prefix.size() != 2)
    {
        setStatusMessage("Configure and select a 2-digit Category before generating a barcode.", true);
        return;
    }
    if (suffix.size() != 4)
    {
        setStatusMessage("Enter exactly 4 digits after the 2-digit category prefix.", true);
        return;
    }

    const QString fullCode = prefix + suffix;
    try
    {
        const std::string bits = Encoder::encode(fullCode.toStdString());
        BarcodeImageGenerator generator;
        m_generatedBarcodeImage = generator.generate(bits);

        m_barcodePreview->setPixmap(QPixmap::fromImage(m_generatedBarcodeImage).scaled(
            m_barcodePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_saveBarcodeBtn->setEnabled(true);

        setStatusMessage("Generated barcode preview for " + fullCode + " (Category " + prefix + "). Click Save Barcode PNG to set image path.");
    }
    catch (const std::exception& e)
    {
        m_saveBarcodeBtn->setEnabled(false);
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void ManagerScreen::onSaveBarcodeImageClicked()
{
    if (m_generatedBarcodeImage.isNull())
        return;

    const QString fullCode = m_barcodePrefixLabel->text().trimmed() + m_barcodeSuffixInput->text().trimmed();
    const QString defaultName = m_imagePathInput->text().trimmed().isEmpty()
                                    ? ("barcode_" + fullCode + ".png")
                                    : m_imagePathInput->text().trimmed();

    const QString path = QFileDialog::getSaveFileName(
        this, "Save Barcode Image", defaultName, "PNG Image (*.png)");
    if (path.isEmpty())
        return;

    if (m_generatedBarcodeImage.save(path, "PNG"))
    {
        QString storedPath = path;
        storedPath.replace(' ', '_');
        m_imagePathInput->setText(storedPath);
        setStatusMessage("Saved barcode image to " + storedPath);
    }
    else
    {
        setStatusMessage("Failed to save barcode image.", true);
    }
}

void ManagerScreen::onLoadSelectedForEditClicked()
{
    const QString productID = selectedCatalogProductID();
    if (productID.isEmpty())
    {
        setStatusMessage("Select a product row in the catalog drawer to edit.", true);
        return;
    }

    Product p;
    if (!m_productService->getProductByID(productID.toStdString(), p))
    {
        setStatusMessage("Could not load selected product for editing.", true);
        return;
    }

    m_isEditingProduct = true;
    m_formGroup->setTitle("1. Edit Product (ID: " + productID + ")");
    m_saveProductBtn->setText("Update Product");
    m_cancelEditBtn->setVisible(true);

    m_idInput->setText(QString::fromStdString(p.productID));
    m_idInput->setReadOnly(true);

    const QString catId = QString::fromStdString(p.categoryID);
    const int catIdx = m_categoryCombo->findData(catId);
    if (catIdx >= 0)
        m_categoryCombo->setCurrentIndex(catIdx);
    m_barcodePrefixLabel->setText(catId);

    if (p.barcode.size() == 6)
        m_barcodeSuffixInput->setText(QString::fromStdString(p.barcode.substr(2, 4)));

    m_acronymInput->setText(QString::fromStdString(p.acronym));
    m_nameInput->setText(QString::fromStdString(p.name));

    const int unitIdx = m_unitCombo->findText(QString::fromStdString(p.unit));
    if (unitIdx >= 0)
        m_unitCombo->setCurrentIndex(unitIdx);

    m_priceSpin->setValue(p.unitPrice);
    m_taxSpin->setValue(p.tax);
    m_discountSpin->setValue(p.discount);
    m_quantitySpin->setValue(p.quantity);
    m_imagePathInput->setText(QString::fromStdString(p.barcodeImagePath));

    // Tuck drawer back in if unpinned so the edit form is immediately visible
    if (m_catalogDrawer && !m_catalogDrawer->isPinned())
        m_catalogDrawer->setExpanded(false, true);

    setStatusMessage("Editing product " + QString::fromStdString(p.name) + ". Modify fields and click Update Product.");
}

void ManagerScreen::onCancelEditClicked()
{
    resetProductForm();
    setStatusMessage("Edit cancelled.");
}

void ManagerScreen::resetProductForm()
{
    m_isEditingProduct = false;
    m_formGroup->setTitle("1. Add New Product to Catalog");
    m_saveProductBtn->setText("Save Product to Catalog");
    m_cancelEditBtn->setVisible(false);

    m_idInput->setReadOnly(false);
    m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));
    m_barcodeSuffixInput->clear();
    m_acronymInput->clear();
    m_nameInput->clear();
    m_priceSpin->setValue(0.0);
    m_taxSpin->setValue(0.0);
    m_discountSpin->setValue(0.0);
    m_quantitySpin->setValue(10);
    m_imagePathInput->clear();
}

void ManagerScreen::onSaveProductClicked()
{
    const QString catID = m_barcodePrefixLabel->text().trimmed();
    const QString suffix = m_barcodeSuffixInput->text().trimmed();

    if (catID.size() != 2)
    {
        setStatusMessage("Please configure and select a Category first.", true);
        return;
    }
    if (suffix.size() != 4)
    {
        setStatusMessage("Enter 4 digits after the category prefix to form a 6-digit barcode.", true);
        return;
    }

    Product p;
    p.productID = m_idInput->text().trimmed().toStdString();
    p.categoryID = catID.toStdString();
    p.barcode = (catID + suffix).toStdString();
    p.acronym = m_acronymInput->text().trimmed().toStdString();
    p.name = m_nameInput->text().trimmed().replace(' ', '_').toStdString();
    p.unit = m_unitCombo->currentText().toStdString();
    p.unitPrice = m_priceSpin->value();
    p.tax = m_taxSpin->value();
    p.discount = m_discountSpin->value();
    p.quantity = m_quantitySpin->value();
    p.barcodeImagePath = m_imagePathInput->text().trimmed().toStdString();

    if (m_isEditingProduct)
    {
        if (m_productService->updateProduct(p))
        {
            refreshProductTable();
            resetProductForm();
            setStatusMessage("Updated product " + QString::fromStdString(p.name) + " (" + QString::fromStdString(p.barcode) + ").");
        }
        else
        {
            setStatusMessage("Failed to update product. Ensure barcode is unique and fields are valid.", true);
        }
    }
    else
    {
        if (m_productService->addProduct(p))
        {
            refreshProductTable();
            resetProductForm();
            setStatusMessage("Added product " + QString::fromStdString(p.name) + " (" + QString::fromStdString(p.barcode) + ").");
        }
        else
        {
            setStatusMessage("Failed to add product. Ensure ID and 6-digit barcode are unique and fields are valid.", true);
        }
    }
}

void ManagerScreen::onRestockClicked()
{
    const QString barcode = selectedCatalogBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select a product row in the catalog drawer first.", true);
        return;
    }

    const int addedQty = m_restockSpin->value();
    if (m_productService->restockProduct(barcode.toStdString(), addedQty))
    {
        refreshProductTable();
        setStatusMessage("Restocked barcode " + barcode + " by +" + QString::number(addedQty) + " units.");
    }
    else
    {
        setStatusMessage("Failed to restock selected product.", true);
    }
}

void ManagerScreen::onRemoveProductClicked()
{
    const QString barcode = selectedCatalogBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select a product row in the catalog drawer to remove.", true);
        return;
    }

    const auto reply = QMessageBox::question(
        this,
        "Remove Product",
        "Remove product with barcode " + barcode + " from Data/Product.txt?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_productService->removeProduct(barcode.toStdString()))
    {
        refreshProductTable();
        if (!m_isEditingProduct)
            m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));
        setStatusMessage("Removed product with barcode " + barcode + ".");
    }
    else
    {
        setStatusMessage("Failed to remove selected product.", true);
    }
}

void ManagerScreen::refreshProductTable()
{
    const std::vector<Product> products = m_productService->getAllProducts();

    m_productTable->setRowCount(0);
    for (const auto& p : products)
    {
        const int row = m_productTable->rowCount();
        m_productTable->insertRow(row);

        m_productTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.productID)));
        m_productTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.categoryID)));
        m_productTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p.barcode)));
        m_productTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(p.acronym)));
        m_productTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(p.name)));
        m_productTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(p.unit)));
        m_productTable->setItem(row, 6, new QTableWidgetItem(QString::number(p.unitPrice, 'f', 2)));
        m_productTable->setItem(row, 7, new QTableWidgetItem(QString::number(p.tax, 'f', 2)));
        m_productTable->setItem(row, 8, new QTableWidgetItem(QString::number(p.discount, 'f', 2)));
        m_productTable->setItem(row, 9, new QTableWidgetItem(QString::number(p.quantity)));
    }
}

void ManagerScreen::refreshSeasonsTable()
{
    const std::vector<SeasonRule> rules = m_seasonRepository.getSeasonRules();
    m_seasonsTable->setRowCount(0);

    for (const auto& r : rules)
    {
        const int row = m_seasonsTable->rowCount();
        m_seasonsTable->insertRow(row);

        m_seasonsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(r.startDate)));
        m_seasonsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.endDate)));
        m_seasonsTable->setItem(row, 2, new QTableWidgetItem(QString::number(r.percentage, 'f', 2) + "%"));
        m_seasonsTable->setItem(row, 3, new QTableWidgetItem(r.isActive ? "Active" : "Inactive"));
    }
}

void ManagerScreen::onAddSeasonClicked()
{
    SeasonRule rule;
    rule.startDate = m_seasonStartInput->text().trimmed().toStdString();
    rule.endDate = m_seasonEndInput->text().trimmed().toStdString();
    rule.percentage = m_seasonPercentSpin->value();
    rule.isActive = m_seasonActiveCheck->isChecked();

    if (!SeasonalDiscountService::isValidDate(rule.startDate) ||
        !SeasonalDiscountService::isValidDate(rule.endDate))
    {
        setStatusMessage("Invalid season dates. Use DD-MM-YYYY format (e.g., 01-11-2026).", true);
        return;
    }

    if (m_seasonRepository.addSeasonRule(rule))
    {
        m_seasonStartInput->clear();
        m_seasonEndInput->clear();
        refreshSeasonsTable();
        setStatusMessage("Added seasonal discount (" + QString::number(rule.percentage, 'f', 2) + "%).");
    }
    else
    {
        setStatusMessage("Failed to save seasonal discount rule.", true);
    }
}

void ManagerScreen::onToggleSeasonActiveClicked()
{
    const int row = m_seasonsTable->currentRow();
    if (row < 0)
    {
        setStatusMessage("Select a season row first to toggle Active/Inactive.", true);
        return;
    }

    std::vector<SeasonRule> rules = m_seasonRepository.getSeasonRules();
    if (row >= static_cast<int>(rules.size()))
        return;

    rules[row].isActive = !rules[row].isActive;
    if (m_seasonRepository.saveAllSeasonRules(rules))
    {
        refreshSeasonsTable();
        setStatusMessage("Updated season status to " + QString(rules[row].isActive ? "Active." : "Inactive."));
    }
    else
    {
        setStatusMessage("Failed to update Data/Seasons.txt.", true);
    }
}

void ManagerScreen::onOpenSavedBillsClicked()
{
    const QString billPath = resolveSiblingDataPath(m_productFilePath, "Bill.txt");
    const QString detailPath = resolveSiblingDataPath(m_productFilePath, "BillDetail.txt");

    FileBillRepository billRepo(billPath.toStdString());
    FileBillDetailRepository detailRepo(detailPath.toStdString());

    std::vector<Bill> bills;
    try
    {
        bills = billRepo.getAllBills();
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "Error Loading Bills", QString::fromUtf8(e.what()));
        return;
    }

    if (bills.empty())
    {
        QMessageBox::information(this, "Saved Bills", "No saved bills found in Data/Bill.txt yet.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Saved Bills - View & Export PDF");
    dlg.resize(760, 420);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    QTableWidget* table = new QTableWidget(0, 7, &dlg);
    table->setHorizontalHeaderLabels(
        QStringList() << "Bill ID" << "Order ID" << "Customer" << "Date" << "Time" << "Payable" << "Status");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (const auto& b : bills)
    {
        const int r = table->rowCount();
        table->insertRow(r);
        table->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(b.billID)));
        table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(b.orderID)));
        table->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(b.customerName)));
        table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(b.billDate)));
        table->setItem(r, 4, new QTableWidgetItem(QString::fromStdString(b.billTime)));
        table->setItem(r, 5, new QTableWidgetItem("PKR " + QString::number(b.totalPayable, 'f', 2)));
        table->setItem(r, 6, new QTableWidgetItem(QString::fromStdString(b.paymentStatus)));
    }
    layout->addWidget(table, 1);

    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* viewPdfBtn = new QPushButton("Open Receipt && Export PDF", &dlg);
    QPushButton* closeBtn = new QPushButton("Close", &dlg);
    btnRow->addWidget(viewPdfBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(viewPdfBtn, &QPushButton::clicked, [&]() {
        const int row = table->currentRow();
        if (row < 0 || row >= static_cast<int>(bills.size()))
        {
            QMessageBox::information(&dlg, "Select Bill", "Please select a bill row first.");
            return;
        }
        Bill selected = bills[row];
        selected.lines = detailRepo.getLinesByBillID(selected.billID);
        Payment summary(selected.totalPayable, "saved", "-");
        BillDialog receiptDlg(selected, summary, selected.amountReceived, &dlg);
        receiptDlg.exec();
    });

    dlg.exec();
}