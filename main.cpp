#include <QCoreApplication>

#include "driver/compilerdriver.h"

int main(int argc, char *argv[])
{
    /*
     * Flags: tabelle dei flag nel file driver/flags.h
     */

    QCoreApplication a(argc, argv);
    a.setApplicationName("Bismuth Compiler for C++");
    a.setApplicationVersion("0.1.1");

    CompilerDriver driver;
    driver.run(a);

    return 0;
}
