#include <QApplication>
#include <QFont>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Mem-Map"));
    app.setApplicationDisplayName(QStringLiteral("Mem-Map"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("Mem-Map"));

    QFont defaultFont(QStringLiteral("Segoe UI"), 10);
    defaultFont.setStyleHint(QFont::SansSerif);
    QApplication::setFont(defaultFont);

    MainWindow window;
    window.show();

    return app.exec();
}
