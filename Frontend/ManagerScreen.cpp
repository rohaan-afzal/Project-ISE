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
#include <QFrame>
#include <stdexcept>
#include <vector>

namespace {
QString resolveSiblingDataPath(const QString& productFilePath, const QString& fileName)
{
    QFileInfo info(productFilePath);
    return info.dir().filePath(fileName);
}
}

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

void ManagerScreen::setupUi()
{
    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setSpacing(12);

    // ================= LEFT COLUMN: Category Config + Add/Edit Product =================
    QVBoxLayout* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(8);

    // 1. Category Configuration Group (Req 1)
    QGroupBox* catGroup = new QGroupBox("1. Product Categories (Data/PCategory.txt)", this);
    QVBoxLayout* catLayout = new QVBoxLayout(catGroup);
    catLayout->setSpacing(6);

    QHBoxLayout* catFormRow = new QHBoxLayout();
    m_catIdInput = new QLineEdit(this);
    m_catIdInput->setPlaceholderText("ID (10-99)");
    m_catIdInput->setMaxLength(2);
    m_catIdInput->setMaximumWidth(80);
    m_catIdInput->setMinimumHeight(26);
    m_catIdInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]"), m_catIdInput));
    m_catIdInput->setText(QString::fromStdString(m_categoryService.getNextCategoryID()));

    m_catNameInput = new QLineEdit(this);
    m_catNameInput->setPlaceholderText("Category Name");
    m_catNameInput->setMinimumHeight(26);

    m_catAcronymInput = new QLineEdit(this);
    m_catAcronymInput->setPlaceholderText("Acronym");
    m_catAcronymInput->setMaximumWidth(95);
    m_catAcronymInput->setMinimumHeight(26);

    m_addCategoryBtn = new QPushButton("Add Category", this);
    m_addCategoryBtn->setMinimumHeight(26);

    catFormRow->addWidget(m_catIdInput);
    catFormRow->addWidget(m_catNameInput);
    catFormRow->addWidget(m_catAcronymInput);
    catFormRow->addWidget(m_addCategoryBtn);
    catLayout->addLayout(catFormRow);

    m_categoryTable = new QTableWidget(0, 3, this);
    m_categoryTable->setHorizontalHeaderLabels(QStringList() << "Category ID" << "Category Name" << "Acronym");
    m_categoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_categoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryTable->verticalHeader()->setVisible(false);
    m_categoryTable->verticalHeader()->setDefaultSectionSize(24);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_categoryTable->setMinimumHeight(105);
    m_categoryTable->setMaximumHeight(130);
    catLayout->addWidget(m_categoryTable);

    leftColumn->addWidget(catGroup);

    // 2. Add / Edit Product + Barcode Generator Group
    m_formGroup = new QGroupBox("2. Add New Product to Catalog", this);
    QVBoxLayout* formGroupLayout = new QVBoxLayout(m_formGroup);
    formGroupLayout->setSpacing(6);

    QFormLayout* form = new QFormLayout();
    form->setVerticalSpacing(5);
    form->setHorizontalSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumHeight(26);

    m_idInput = new QLineEdit(this);
    m_idInput->setMinimumHeight(26);
    m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));

    QHBoxLayout* barcodeFieldRow = new QHBoxLayout();
    barcodeFieldRow->setSpacing(6);
    m_barcodePrefixLabel = new QLineEdit(this);
    m_barcodePrefixLabel->setReadOnly(true);
    m_barcodePrefixLabel->setFixedWidth(48);
    m_barcodePrefixLabel->setMinimumHeight(26);
    m_barcodePrefixLabel->setAlignment(Qt::AlignCenter);
    m_barcodePrefixLabel->setStyleSheet("background-color: #e9ecef; font-weight: bold;");

    m_barcodeSuffixInput = new QLineEdit(this);
    m_barcodeSuffixInput->setPlaceholderText("4 digits (e.g. 0001)");
    m_barcodeSuffixInput->setMaxLength(4);
    m_barcodeSuffixInput->setMinimumHeight(26);
    m_barcodeSuffixInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[0-9]{0,4}"), m_barcodeSuffixInput));

    barcodeFieldRow->addWidget(m_barcodePrefixLabel);
    barcodeFieldRow->addWidget(m_barcodeSuffixInput);

    m_acronymInput = new QLineEdit(this);
    m_acronymInput->setPlaceholderText("e.g. TEA");
    m_acronymInput->setMinimumHeight(26);

    m_nameInput = new QLineEdit(this);
    m_nameInput->setPlaceholderText("e.g. GreenTea");
    m_nameInput->setMinimumHeight(26);

    m_unitCombo = new QComboBox(this);
    m_unitCombo->setMinimumHeight(26);
    m_unitCombo->addItems(QStringList() << "Pack" << "Kg" << "Gram" << "Litre" << "ml" << "Piece" << "Box" << "Bottle" << "Dozen");

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.0, 1000000.0);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setPrefix("PKR ");
    m_priceSpin->setMinimumHeight(26);

    m_taxSpin = new QDoubleSpinBox(this);
    m_taxSpin->setRange(0.0, 100000.0);
    m_taxSpin->setDecimals(2);
    m_taxSpin->setMinimumHeight(26);

    m_discountSpin = new QDoubleSpinBox(this);
    m_discountSpin->setRange(0.0, 100000.0);
    m_discountSpin->setDecimals(2);
    m_discountSpin->setMinimumHeight(26);

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(0, 100000);
    m_quantitySpin->setValue(10);
    m_quantitySpin->setMinimumHeight(26);

    m_imagePathInput = new QLineEdit(this);
    m_imagePathInput->setPlaceholderText("Click Save Barcode PNG to record saved file path");
    m_imagePathInput->setMinimumHeight(26);

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
    m_genBarcodeBtn->setMinimumHeight(28);
    m_saveBarcodeBtn = new QPushButton("Save Barcode PNG", this);
    m_saveBarcodeBtn->setMinimumHeight(28);
    m_saveBarcodeBtn->setEnabled(false);
    barcodeBtns->addWidget(m_genBarcodeBtn);
    barcodeBtns->addWidget(m_saveBarcodeBtn);
    formGroupLayout->addLayout(barcodeBtns);

    m_barcodePreview = new QLabel("Barcode preview", this);
    m_barcodePreview->setAlignment(Qt::AlignCenter);
    m_barcodePreview->setMinimumHeight(55);
    m_barcodePreview->setMaximumHeight(65);
    m_barcodePreview->setFrameShape(QFrame::Box);
    formGroupLayout->addWidget(m_barcodePreview);

    QHBoxLayout* saveActionBtns = new QHBoxLayout();
    m_saveProductBtn = new QPushButton("Save Product to Catalog", this);
    m_saveProductBtn->setMinimumHeight(30);
    m_cancelEditBtn = new QPushButton("Cancel Edit", this);
    m_cancelEditBtn->setMinimumHeight(30);
    m_cancelEditBtn->setVisible(false);
    saveActionBtns->addWidget(m_saveProductBtn);
    saveActionBtns->addWidget(m_cancelEditBtn);
    formGroupLayout->addLayout(saveActionBtns);

    m_statusLabel = new QLabel("Ready.", this);
    m_statusLabel->setWordWrap(true);
    formGroupLayout->addWidget(m_statusLabel);

    leftColumn->addWidget(m_formGroup, 1);
    rootLayout->addLayout(leftColumn, 4);

    // ================= RIGHT COLUMN: Catalog + Seasonal Discounts + Saved Bills PDF =================
    QVBoxLayout* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(8);

    QGroupBox* catalogGroup = new QGroupBox("3. Current Product Catalog & Inventory (Data/Product.txt)", this);
    QVBoxLayout* rightLayout = new QVBoxLayout(catalogGroup);

    m_productTable = new QTableWidget(0, 10, this);
    m_productTable->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Cat ID" << "Barcode" << "Acronym" << "Name" << "Unit"
                      << "Price" << "Tax" << "Discount" << "Stock");
    m_productTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_productTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_productTable->verticalHeader()->setVisible(false);
    m_productTable->verticalHeader()->setDefaultSectionSize(25);
    m_productTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rightLayout->addWidget(m_productTable, 1);

    QHBoxLayout* stockActionsRow = new QHBoxLayout();
    m_editProductBtn = new QPushButton("Edit Selected", this);
    m_editProductBtn->setMinimumHeight(28);

    m_restockSpin = new QSpinBox(this);
    m_restockSpin->setRange(1, 100000);
    m_restockSpin->setValue(10);
    m_restockSpin->setPrefix("+ Stock: ");
    m_restockSpin->setMinimumHeight(28);

    m_restockBtn = new QPushButton("Restock", this);
    m_restockBtn->setMinimumHeight(28);
    m_removeProductBtn = new QPushButton("Remove", this);
    m_removeProductBtn->setMinimumHeight(28);
    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setMinimumHeight(28);

    stockActionsRow->addWidget(m_editProductBtn);
    stockActionsRow->addWidget(m_restockSpin);
    stockActionsRow->addWidget(m_restockBtn);
    stockActionsRow->addWidget(m_removeProductBtn);
    stockActionsRow->addWidget(m_refreshBtn);
    rightLayout->addLayout(stockActionsRow);

    rightColumn->addWidget(catalogGroup, 3);

    // 4. Seasonal Discounts & Saved Bills PDF Group (Batch 2)
    QGroupBox* seasonsGroup = new QGroupBox("4. Seasonal Discounts (Data/Seasons.txt) & Saved Bills PDF", this);
    QVBoxLayout* seasonsLayout = new QVBoxLayout(seasonsGroup);
    seasonsLayout->setSpacing(6);

    QHBoxLayout* seasonFormRow = new QHBoxLayout();
    m_seasonStartInput = new QLineEdit(this);
    m_seasonStartInput->setPlaceholderText("Start DD-MM-YYYY");
    m_seasonStartInput->setMaxLength(10);
    m_seasonStartInput->setMinimumHeight(26);

    m_seasonEndInput = new QLineEdit(this);
    m_seasonEndInput->setPlaceholderText("End DD-MM-YYYY");
    m_seasonEndInput->setMaxLength(10);
    m_seasonEndInput->setMinimumHeight(26);

    m_seasonPercentSpin = new QDoubleSpinBox(this);
    m_seasonPercentSpin->setRange(0.0, 100.0);
    m_seasonPercentSpin->setDecimals(2);
    m_seasonPercentSpin->setSuffix(" %");
    m_seasonPercentSpin->setValue(10.0);
    m_seasonPercentSpin->setMinimumHeight(26);

    m_seasonActiveCheck = new QCheckBox("Active", this);
    m_seasonActiveCheck->setChecked(true);

    m_addSeasonBtn = new QPushButton("Add Season", this);
    m_addSeasonBtn->setMinimumHeight(26);

    seasonFormRow->addWidget(m_seasonStartInput);
    seasonFormRow->addWidget(m_seasonEndInput);
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
    m_seasonsTable->verticalHeader()->setDefaultSectionSize(24);
    m_seasonsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_seasonsTable->setMinimumHeight(95);
    m_seasonsTable->setMaximumHeight(125);
    seasonsLayout->addWidget(m_seasonsTable);

    QHBoxLayout* seasonActionsRow = new QHBoxLayout();
    m_toggleSeasonBtn = new QPushButton("Toggle Selected Season Active/Inactive", this);
    m_toggleSeasonBtn->setMinimumHeight(28);
    m_viewBillsBtn = new QPushButton("View / Print Saved Bills (PDF)", this);
    m_viewBillsBtn->setMinimumHeight(28);
    seasonActionsRow->addWidget(m_toggleSeasonBtn);
    seasonActionsRow->addStretch();
    seasonActionsRow->addWidget(m_viewBillsBtn);
    seasonsLayout->addLayout(seasonActionsRow);

    rightColumn->addWidget(seasonsGroup, 2);
    rootLayout->addLayout(rightColumn, 6);

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
        setStatusMessage("Select a product row in the catalog table to edit.", true);
        return;
    }

    Product p;
    if (!m_productService->getProductByID(productID.toStdString(), p))
    {
        setStatusMessage("Could not load selected product for editing.", true);
        return;
    }

    m_isEditingProduct = true;
    m_formGroup->setTitle("2. Edit Product (ID: " + productID + ")");
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
    m_formGroup->setTitle("2. Add New Product to Catalog");
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
        setStatusMessage("Select a product row in the catalog table first.", true);
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
        setStatusMessage("Select a product row in the catalog table to remove.", true);
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