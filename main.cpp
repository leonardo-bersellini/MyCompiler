#include "driver/compilerdriver.h"

int main(int argc, char *argv[])
{
    /*
     * Flags: tabelle dei flag nel file driver/flags.h
     */

    CompilerDriver driver;
    driver.run(argc, argv);

    return 0;
}
