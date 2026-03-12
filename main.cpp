#include <QApplication>
#include "AppStyle.h"
#include "ui/MainWindow.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("NestingApp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("NestingPro");
    app.setStyleSheet(appStyleSheet());

    AppLogger::instance().info(LogChannel::SYSTEM, "NestingApp запущен",
                               QString("Qt %1, версия приложения %2")
                               .arg(qVersion()).arg(app.applicationVersion()));

    MainWindow window;
    window.show();

    const int ret = app.exec();
    AppLogger::instance().info(LogChannel::SYSTEM, "NestingApp завершён",
                               QString("Код возврата: %1").arg(ret));
    return ret;
}
