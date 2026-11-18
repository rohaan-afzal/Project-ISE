#pragma once
#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QString>
#include <atomic>
#include <memory>
#include <thread>

class QLabel;
class QPushButton;
class CameraBarcodeReader;

class CameraWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(QWidget* parent = nullptr);
    ~CameraWidget() override;

    void stopScanning();
    bool isScanning() const { return m_scanning; }

signals:
    void barcodeScanned(const QString& barcode);
    void scanError(const QString& message);

private slots:
    void onCameraButtonClicked();
    void onUploadButtonClicked();

private:
    QPushButton* m_cameraBtn;
    QPushButton* m_uploadBtn;
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;

    std::unique_ptr<CameraBarcodeReader> m_reader;
    bool m_scanning;
    std::atomic_bool m_cancelScan;
    std::thread m_scanThread;
};

#endif