#include "AnalysisWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AnalysisWindow w;
    w.show();

    return a.exec();
}
