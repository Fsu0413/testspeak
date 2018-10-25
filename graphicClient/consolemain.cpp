#include "console.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
#ifdef Q_OS_ANDROID
    QDir::setCurrent(QStringLiteral("/sdcard/graphicClient/"));
#else
    //QDir::setCurrent(a.applicationDirPath());
#endif

    Dialog w;
    Q_UNUSED(w);
    return a.exec();
}
