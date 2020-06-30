#include "dialog.h"
#include <QDebug>
#include <QDir>
#ifdef GRAPHICSCLIENT
#include <QApplication>
#include <QStyleFactory>
#else
#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QCoreApplication>
#endif

#endif

int main(int argc, char *argv[])
{
#ifdef GRAPHICSCLIENT
    QApplication a(argc, argv);
    a.setStyle(QStringLiteral("Fusion"));
#else
#ifdef Q_OS_ANDROID
    QGuiApplication a(argc, argv);
#else
    QCoreApplication a(argc, argv);
#endif
#endif

#ifdef Q_OS_ANDROID
    QDir::setCurrent(QStringLiteral("/sdcard/Android/data/org.qtproject.example.graphicClient"));
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
