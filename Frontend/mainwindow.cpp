
#include "mainwindow.h"

#include "Encoder/Encoder.h"
#include "BarcodeImageGenerator.h"
#include "../Backend/Decoder/Decoder.h"
#include "../Backend/ImageProcessor/ImageProcessor.h"
#include "../Backend/BarcodeReader/CameraBarcodeReader.h"
#include "../Backend/Product/ProductService.h"

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
#include <QMessageBox>
#include <QFrame>
#include <QFont>
#include <QCoreApplication>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>

#include <stdexcept>
#include <string>
#include <vector>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent),
    m_barcodeReader(std::make_unique<CameraBarcodeReader>()), m_scanning(false), m_cancelScan(false)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString catalogPath = QDir::current().filePath("Data/Product.txt");
    if (!QFileInfo::exists(catalogPath)) catalogPath = QDir(appDir).filePath("../Data/Product.txt");
    m_productService = std::make_unique<ProductService>(catalogPath.toStdString());
    setWindowTitle("Barcode Generator & Reader");
    resize(700, 500);

    m_tabs = new QTabWidget(this);

    m_tabs->addTab(createGenerateTab(), "Generate");
    m_tabs->addTab(createReadTab(), "Read");

    setCentralWidget(m_tabs);

    setStyleSheet(R"(
        QMainWindow, QWidget {
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

        QLineEdit {
            background-color: #FFFFFF;
            color: #141413;
            border: 1px solid #B0AEA5;
            border-radius: 6px;
            padding: 6px;
        }

        QLineEdit:focus {
            border: 1px solid #D97757;
        }

        QLabel {
            background-color: transparent;
        }
    )");
}

MainWindow::~MainWindow()
{
    m_cancelScan = true;
    if (m_scanThread.joinable()) m_scanThread.join();
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

    connect(
        m_Generate,
        &QPushButton::clicked,
        this,
        &MainWindow::onGenerateClicked
        );

    connect(
        m_Input,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::onGenerateClicked
        );

    connect(
        m_saveBtn,
        &QPushButton::clicked,
        this,
        &MainWindow::onSaveClicked
        );

    return page;
}

QWidget* MainWindow::createReadTab()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* actions = new QHBoxLayout;
    m_cameraBtn = new QPushButton("Scan with Camera");
    m_uploadBtn = new QPushButton("Upload Barcode Image");
    m_descriptionBtn = new QPushButton("Description");
    m_descriptionBtn->setEnabled(false);
    actions->addWidget(m_cameraBtn);
    actions->addWidget(m_uploadBtn);
    actions->addWidget(m_descriptionBtn);
    layout->addLayout(actions);

    m_readPreview = new QLabel("Uploaded barcode will appear here");
    m_readPreview->setAlignment(Qt::AlignCenter);
    m_readPreview->setMinimumSize(400, 200);
    m_readPreview->setFrameShape(QFrame::Box);

    layout->addWidget(m_readPreview, 1);

    m_decodedLabel = new QLabel("Decoded number: -");

    QFont font = m_decodedLabel->font();
    font.setPointSize(16);
    font.setBold(true);

    m_decodedLabel->setFont(font);
    layout->addWidget(m_decodedLabel);

    m_readStatus = new QLabel;
    layout->addWidget(m_readStatus);

    m_productDetails = new QLabel("Scan a product barcode to view its description.");
    m_productDetails->setWordWrap(true);
    m_productDetails->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(m_productDetails);

    connect(
        m_uploadBtn,
        &QPushButton::clicked,
        this,
        &MainWindow::onUploadClicked
        );
    connect(m_cameraBtn, &QPushButton::clicked, this, &MainWindow::onCameraClicked);
    connect(m_descriptionBtn, &QPushButton::clicked, this, &MainWindow::onDescriptionClicked);

    return page;
}

void MainWindow::onGenerateClicked()
{
    const QString text = m_Input->text();

    try
    {
        std::string bits = Encoder::encode(text.toStdString());
        qDebug() << "bits:" << bits.length() << QString::fromStdString(bits);
        qDebug() << "round trip:" << QString::fromStdString(Decoder::decodeBinary(bits));
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
    {
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        "Save barcode image",
        "barcode_" + m_Input->text() + ".png",
        "PNG image (*.png)"
        );

    if (path.isEmpty())
    {
        return;
    }

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

void MainWindow::onUploadClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Open barcode image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)"
        );

    if (path.isEmpty())
    {
        return;
    }

    QImage img(path);

    if (img.isNull())
    {
        m_decodedLabel->setText("Decoded number: -");
        m_readStatus->setStyleSheet("color: red;");
        m_readStatus->setText("Error: could not open that image.");
        return;
    }

    m_readPreview->setPixmap(
        QPixmap::fromImage(img).scaled(
            m_readPreview->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            )
        );

    m_decodedLabel->setText("Decoded number: -");
    m_readStatus->setStyleSheet("color: black;");
    m_readStatus->setText("Processing image...");

    try
    {
        cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
        if (image.empty()) throw std::runtime_error("Could not process the uploaded image.");
        const std::string decodedNumber = m_barcodeReader->read(image);

        m_decodedLabel->setText(
            "Decoded number: " +
            QString::fromStdString(decodedNumber)
            );

        m_readStatus->setStyleSheet("color: green;");
        m_readStatus->setText("Barcode decoded successfully.");
        showProductForBarcode(QString::fromStdString(decodedNumber));
    }
    catch (const std::exception& e)
    {
        m_decodedLabel->setText("Decoded number: -");

        m_readStatus->setStyleSheet("color: red;");
        m_readStatus->setText(QString::fromStdString(e.what()));
    }
}

void MainWindow::onCameraClicked()
{
    if (m_scanning) {
        m_cancelScan = true;
        m_cameraBtn->setText("Cancelling…");
        return;
    }
    m_scanning = true;
    m_cancelScan = false;
    m_cameraBtn->setText("Cancel scan");
    m_cameraBtn->setEnabled(true);
    m_uploadBtn->setEnabled(false);
    m_readStatus->setStyleSheet("color: black;");
    m_readStatus->setText("Scanning continuously. Hold the barcode in view or cancel.");
    if (m_scanThread.joinable()) m_scanThread.join();
    m_scanThread = std::thread([this] {
        QString result;
        QString error;
        try {
            const std::string code = m_barcodeReader->readFromCamera(
                [this](const cv::Mat& frame) {
                    // Convert on the worker and queue only the finished image for UI rendering.
                    const QImage preview = ImageProcessor::matToQImage(frame).copy();
                    QMetaObject::invokeMethod(this, [this, preview] {
                        m_readPreview->setPixmap(QPixmap::fromImage(preview).scaled(
                            m_readPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
                    }, Qt::QueuedConnection);
                },
                [this] { return m_cancelScan.load(); });
            result = QString::fromStdString(code);
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }

        QMetaObject::invokeMethod(this, [this, result, error] {
            if (!result.isEmpty()) {
                m_decodedLabel->setText("Decoded number: " + result);
                m_readStatus->setStyleSheet("color: green;");
                m_readStatus->setText("Barcode decoded from camera.");
                showProductForBarcode(result);
            } else {
                const bool cancelled = m_cancelScan.load();
                m_decodedLabel->setText("Decoded number: -");
                m_readStatus->setStyleSheet(cancelled ? "color: black;" : "color: red;");
                m_readStatus->setText(cancelled ? "Barcode scan cancelled." : error);
            }
            m_scanning = false;
            m_cameraBtn->setText("Scan with Camera");
            m_cameraBtn->setEnabled(true);
            m_uploadBtn->setEnabled(true);
            if (m_scanThread.joinable()) m_scanThread.join();
        }, Qt::QueuedConnection);
    });
}

void MainWindow::showProductForBarcode(const QString& barcode)
{
    m_scannedBarcode = barcode;
    m_descriptionBtn->setEnabled(true);
    onDescriptionClicked();
}

void MainWindow::onDescriptionClicked()
{
    if (m_scannedBarcode.isEmpty()) return;
    try {
        const auto product = m_productService->findByBarcode(m_scannedBarcode.toStdString());
        if (!product) {
            m_productDetails->setText("No product found for barcode " + m_scannedBarcode + ".");
            return;
        }
        m_productDetails->setText(
            "<b>" + QString::fromStdString(product->name).toHtmlEscaped() + "</b><br>" +
            QString::fromStdString(product->description).toHtmlEscaped() + "<br><b>Specifications:</b> " +
            QString::fromStdString(product->specifications).toHtmlEscaped());
    } catch (const std::exception& e) {
        m_productDetails->setText("Product lookup failed: " + QString::fromUtf8(e.what()));
    }
}
