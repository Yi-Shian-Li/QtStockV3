#include "QtStockV3.h"
#include <QtWidgets/QApplication>
#include <QtGlobal>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtStockV3 w;
    w.show();

    return a.exec();
}
