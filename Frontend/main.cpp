#include "../Frontend/mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <exception>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    try
    {
        MainWindow w;
        w.show();
        return a.exec();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr, "Startup / Runtime Exception", QString::fromUtf8(e.what()));
        return 1;
    }
    catch (...)
    {
        QMessageBox::critical(nullptr, "Startup / Runtime Exception", "Unknown non-standard C++ exception thrown.");
        return 1;
    }
}