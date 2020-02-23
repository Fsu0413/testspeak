#include "dialog.h"
#include <QDebug>
#include <QDir>
#ifdef GRAPHICSCLIENT
#include <QApplication>
#include <QStyleFactory>
#else
#include <QCoreApplication>
#endif

int main(int argc, char *argv[])
{
#ifdef GRAPHICSCLIENT
    QApplication a(argc, argv);
    a.setStyle(QStringLiteral("Fusion"));
#else
    QCoreApplication a(argc, argv);
#endif

#ifdef Q_OS_ANDROID
    QDir::setCurrent(QStringLiteral("/sdcard/graphicClient/"));
#else
    QDir::setCurrent(a.applicationDirPath());
#endif

    Dialog w;
#ifdef GRAPHICSCLIENT
#ifdef Q_OS_ANDROID
    w.showMaximized();
#else
    w.show();
#endif
#else
    Q_UNUSED(w);
#endif
    return a.exec();
}
