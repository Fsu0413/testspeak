#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
#ifdef Q_OS_ANDROID
    w.showMaximized();
#else
    w.show();
#endif
    return a.exec();
}
