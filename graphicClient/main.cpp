#include "dialog.h"
#include <QApplication>
#include <QDir>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qDebug() << QStyleFactory::keys();
    a.setStyle("Fusion");
#ifdef Q_OS_ANDROID
    QDir::setCurrent("/sdcard/graphicClient/");
#else
    QDir::setCurrent(a.applicationDirPath());
#endif

    Dialog w;
#ifdef Q_OS_ANDROID
    w.showMaximized();
#else
    w.show();
#endif
    return a.exec();
}
