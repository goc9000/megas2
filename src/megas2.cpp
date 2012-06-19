#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "i2c_bus.h"
#include "atmega32.h"
#include "ds1307.h"
#include "fail.h"

using namespace std;

int main(int argc, char **argv)
{
    try {
        if (argc < 2) {
            printf("Invocation: %s <file.elf>\n", argv[0]);
            exit(EXIT_SUCCESS);
        }

        Atmega32 mcu;
        Ds1307 rtc(0x68);
        I2cBus i2c_bus;
        
        rtc.connectToI2cBus(&i2c_bus);
        
        mcu.connectToI2cBus(&i2c_bus);
        mcu.load_program_from_elf(argv[1]);

        while (true)
            mcu.step();
        
        fail("PAF");
    } catch (exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
