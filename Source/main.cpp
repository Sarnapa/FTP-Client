#include <QtGui/QApplication>
#include "window.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    window w;
#if defined(Q_WS_S60)
    w.showMaximized();
#else
    w.show();
#endif
    return a.exec();
}
