#include "mainwindow.h"
#include "CashierScreen.h"
#include "ManagerScreen.h"

#include "Encoder/Encoder.h"
#include "BarcodeImageGenerator.h"
#include "../Backend/Product/ProductService.h"
#include "../Backend/Order/OrderRepository.h"
#include "../Backend/Order/OrderService.h"
#include "../Backend/Billing/BillRepo3.h"
#include "../Backend/Billing/BillCalc3.h"
#include "../Backend/Billing/SeasonRepository.h"
#include "../Frontend/ReportsScreen.h"

#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPixmap>
#include <QFileDialog>
#include <QFrame>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <stdexcept>
#include <string>

QString MainWindow::resolveDataDirectory() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::current().filePath("Data"),
        QDir(appDir).filePath("Data"),
        QDir(appDir).filePath("../Data"),
        QDir(appDir).filePath("../../Data")
    };

    for (const QString& path : candidates)
    {
        if (QFileInfo::exists(QDir(path).filePath("Product.txt")))
            return QDir(path).absolutePath();
    }

    QDir fallback(QDir::current().filePath("Data"));
    if (!fallback.exists())
        fallback.mkpath(".");
    return fallback.absolutePath();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    m_cashierScreen(nullptr),
    m_managerScreen(nullptr)
{
    m_dataDirPath = resolveDataDirectory();
    const QDir dataDir(m_dataDirPath);

    const QString productPath = dataDir.filePath("Product.txt");
    const QString orderPath = dataDir.filePath("Order.txt");
    const QString orderDetailPath = dataDir.filePath("OrderDetail.txt");
    const QString billPath = dataDir.filePath("Bill.txt");
    const QString billDetailPath = dataDir.filePath("BillDetail.txt");
    const QString seasonsPath = dataDir.filePath("Seasons.txt");

    m_productService = std::make_unique<ProductService>(productPath.toStdString());
    m_orderRepository = std::make_unique<OrderRepository>(orderPath.toStdString());
    m_orderDetailRepository = std::make_unique<OrderDetailRepository>(orderDetailPath.toStdString());
    m_billRepository = std::make_unique<FileBillRepository>(billPath.toStdString());
    m_billDetailRepository = std::make_unique<FileBillDetailRepository>(billDetailPath.toStdString());
    m_seasonRepository = std::make_unique<FileSeasonRepository>(seasonsPath.toStdString());
    m_seasonalDiscountService = std::make_unique<SeasonalDiscountService>(*m_seasonRepository);
    m_billService = std::make_unique<BillService>(
        *m_billRepository, *m_billDetailRepository, *m_seasonalDiscountService);
    m_orderService = std::make_unique<OrderService>(
        *m_productService, *m_orderRepository, *m_orderDetailRepository, *m_billService, m_seasonalDiscountService.get());

    setWindowTitle("POS System - Checkout, Inventory & Barcode Generator");
    resize(1120, 720);

    m_tabs = new QTabWidget(this);

    m_cashierScreen = new CashierScreen(m_productService.get(), m_orderService.get(), this);
    m_managerScreen = new ManagerScreen(m_productService.get(), productPath, this);
    m_reportsScreen = new ReportsScreen(m_dataDirPath, this);

    m_tabs->addTab(m_cashierScreen, "Cashier POS");
    m_tabs->addTab(m_managerScreen, "Manager Catalog");
    m_tabs->addTab(m_reportsScreen, "Reports");
    m_tabs->addTab(createGenerateTab(), "Generate");

    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    setCentralWidget(m_tabs);

    setStyleSheet(R"(
        QMainWindow, QWidget, QDialog {
            background-color: #F5F4ED;
            color: #141413;
            font-size: 14px;
        }

        QTabWidget::pane {
            border: 1px solid #B0AEA5;
            border-radius: 6px;
            background-color: #F5F4ED;
        }

        QTabBar::tab {
            background-color: #E8E6DC;
            color: #141413;
            padding: 8px 20px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            margin-right: 2px;
        }

        QTabBar::tab:selected {
            background-color: #D97757;
            color: #FFFFFF;
        }

        QTabBar::tab:hover:!selected {
            background-color: #DDD9CC;
        }

        QGroupBox {
            border: 1px solid #B0AEA5;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: bold;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }

        QPushButton {
            background-color: #D97757;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
        }

        QPushButton:hover {
            background-color: #C6613F;
        }

        QPushButton:pressed {
            background-color: #B5532F;
        }

        QPushButton:disabled {
            background-color: #E8E6DC;
            color: #B0AEA5;
        }

        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: #FFFFFF;
            color: #141413;
            border: 1px solid #B0AEA5;
            border-radius: 6px;
            padding: 6px;
        }

        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 2px solid #D97757;
        }

        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #141413;
            selection-background-color: #D97757;
            selection-color: #FFFFFF;
        }

        QTableWidget {
            background-color: #FFFFFF;
            alternate-background-color: #FAF9F5;
            color: #141413;
            gridline-color: #DDD9CC;
            border: 1px solid #B0AEA5;
            border-radius: 6px;
            selection-background-color: #D97757;
            selection-color: #FFFFFF;
            outline: 0;
        }

        QTableWidget::item:selected {
            background-color: #D97757;
            color: #FFFFFF;
        }

        QTableWidget::item:hover:!selected {
            background-color: #F2DDD4;
            color: #141413;
        }

        QHeaderView::section {
            background-color: #E8E6DC;
            color: #141413;
            padding: 6px;
            border: none;
            border-right: 1px solid #B0AEA5;
            border-bottom: 1px solid #B0AEA5;
            font-weight: bold;
        }

        QLabel {
            background-color: transparent;
        }
    )");
}

MainWindow::~MainWindow()
{
    if (m_cashierScreen)
        m_cashierScreen->stopCamera();
}

void MainWindow::onTabChanged(int index)
{
    if (m_tabs->widget(index) != m_cashierScreen && m_cashierScreen)
        m_cashierScreen->stopCamera();

    if (m_tabs->widget(index) == m_managerScreen && m_managerScreen)
        m_managerScreen->refreshProductTable();

    if (m_tabs->widget(index) == m_reportsScreen && m_reportsScreen)
        m_reportsScreen->refreshAllReports();
}

QWidget* MainWindow::createGenerateTab()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* inputRow = new QHBoxLayout;

    inputRow->addWidget(new QLabel("6-digit number:"));

    m_Input = new QLineEdit;
    m_Input->setPlaceholderText("e.g. 123456 (first digit 1-9)");
    m_Input->setMaxLength(6);

    m_Input->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("[1-9][0-9]{0,5}"),
            m_Input
            )
        );

    inputRow->addWidget(m_Input, 1);

    m_Generate = new QPushButton("Generate");
    inputRow->addWidget(m_Generate);

    layout->addLayout(inputRow);

    m_Preview = new QLabel("Barcode preview will appear here");
    m_Preview->setAlignment(Qt::AlignCenter);
    m_Preview->setMinimumSize(400, 200);
    m_Preview->setFrameShape(QFrame::Box);

    layout->addWidget(m_Preview, 1);

    m_saveBtn = new QPushButton("Save Image");
    m_saveBtn->setEnabled(false);

    layout->addWidget(m_saveBtn);

    m_Status = new QLabel;
    layout->addWidget(m_Status);

    connect(m_Generate, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
    connect(m_Input, &QLineEdit::returnPressed, this, &MainWindow::onGenerateClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);

    return page;
}

void MainWindow::onGenerateClicked()
{
    const QString text = m_Input->text();

    try
    {
        std::string bits = Encoder::encode(text.toStdString());
        BarcodeImageGenerator generator;

        m_currentImage = generator.generate(bits);

        m_Preview->setPixmap(
            QPixmap::fromImage(m_currentImage).scaled(
                m_Preview->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                )
            );

        m_saveBtn->setEnabled(true);

        m_Status->setStyleSheet("color: green;");
        m_Status->setText("Barcode generated for " + text);
    }
    catch (const std::exception& e)
    {
        m_saveBtn->setEnabled(false);

        m_Status->setStyleSheet("color: red;");
        m_Status->setText(QString("Error: ") + e.what());
    }
}

void MainWindow::onSaveClicked()
{
    if (m_currentImage.isNull())
        return;

    const QString path = QFileDialog::getSaveFileName(
        this,
        "Save barcode image",
        "barcode_" + m_Input->text() + ".png",
        "PNG image (*.png)"
        );

    if (path.isEmpty())
        return;

    if (m_currentImage.save(path, "PNG"))
    {
        m_Status->setStyleSheet("color: green;");
        m_Status->setText("Saved to " + path);
    }
    else
    {
        m_Status->setStyleSheet("color: red;");
        m_Status->setText("Error: could not save the image.");
    }
}