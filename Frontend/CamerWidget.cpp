#include "../Backend/BarcodeReader/CameraBarcodeReader.h"
#include "CameraWidget.h"
#include "../Backend/ImageProcessor/ImageProcessor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMetaObject>
#include <QFrame>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>

CameraWidget::CameraWidget(QWidget* parent)
    : QWidget(parent),
    m_reader(std::make_unique<CameraBarcodeReader>()),
    m_scanning(false),
    m_cancelScan(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    m_cameraBtn = new QPushButton("Scan with Camera", this);
    m_uploadBtn = new QPushButton("Upload Image", this);
    buttonRow->addWidget(m_cameraBtn);
    buttonRow->addWidget(m_uploadBtn);
    layout->addLayout(buttonRow);

    m_previewLabel = new QLabel("Camera / barcode preview", this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(280, 160);
    m_previewLabel->setFrameShape(QFrame::Box);
    layout->addWidget(m_previewLabel, 1);

    m_statusLabel = new QLabel("Ready to scan.", this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    connect(m_cameraBtn, &QPushButton::clicked, this, &CameraWidget::onCameraButtonClicked);
    connect(m_uploadBtn, &QPushButton::clicked, this, &CameraWidget::onUploadButtonClicked);
}

CameraWidget::~CameraWidget()
{
    stopScanning();
}

void CameraWidget::stopScanning()
{
    m_cancelScan = true;
    if (m_scanThread.joinable())
        m_scanThread.join();
    m_scanning = false;
}

void CameraWidget::onCameraButtonClicked()
{
    if (m_scanning)
    {
        m_cancelScan = true;
        m_cameraBtn->setText("Cancelling...");
        return;
    }

    m_scanning = true;
    m_cancelScan = false;
    m_cameraBtn->setText("Stop Camera");
    m_uploadBtn->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #141413;");
    m_statusLabel->setText("Scanning... Hold a 6-digit barcode in front of the camera.");

    if (m_scanThread.joinable())
        m_scanThread.join();

    m_scanThread = std::thread([this] {
        QString scanned;
        QString errorText;

        try
        {
            const std::string code = m_reader->readFromCamera(
                [this](const cv::Mat& frame) {
                    const QImage preview = ImageProcessor::matToQImage(frame).copy();
                    QMetaObject::invokeMethod(this, [this, preview] {
                        m_previewLabel->setPixmap(QPixmap::fromImage(preview).scaled(
                            m_previewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
                        }, Qt::QueuedConnection);
                },
                [this] { return m_cancelScan.load(); });

            scanned = QString::fromStdString(code);
        }
        catch (const std::exception& e)
        {
            errorText = QString::fromUtf8(e.what());
        }

        QMetaObject::invokeMethod(this, [this, scanned, errorText] {
            if (!scanned.isEmpty())
            {
                m_statusLabel->setStyleSheet("color: green;");
                m_statusLabel->setText("Scanned barcode: " + scanned);
                emit barcodeScanned(scanned);
            }
            else
            {
                const bool cancelled = m_cancelScan.load();
                m_statusLabel->setStyleSheet(cancelled ? "color: #141413;" : "color: red;");
                m_statusLabel->setText(cancelled ? "Camera scan stopped." : errorText);
                if (!cancelled && !errorText.isEmpty())
                    emit scanError(errorText);
            }

            m_scanning = false;
            m_cameraBtn->setText("Scan with Camera");
            m_uploadBtn->setEnabled(true);
            if (m_scanThread.joinable())
                m_scanThread.join();
            }, Qt::QueuedConnection);
        });
}

void CameraWidget::onUploadButtonClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Open Barcode Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");

    if (path.isEmpty())
        return;

    QImage img(path);
    if (img.isNull())
    {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("Could not open the selected image.");
        return;
    }

    m_previewLabel->setPixmap(QPixmap::fromImage(img).scaled(
        m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    try
    {
        cv::Mat mat = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
        if (mat.empty())
            throw std::runtime_error("Failed to decode image.");

        const std::string code = m_reader->read(mat);
        const QString qCode = QString::fromStdString(code);

        m_statusLabel->setStyleSheet("color: green;");
        m_statusLabel->setText("Decoded barcode: " + qCode);
        emit barcodeScanned(qCode);
    }
    catch (const std::exception& e)
    {
        const QString err = QString::fromUtf8(e.what());
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText(err);
        emit scanError(err);
    }
}