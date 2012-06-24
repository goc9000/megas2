#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "glue/i2c_bus.h"
#include "glue/spi_bus.h"
#include "glue/ss_pin_monitor.h"
#include "glue/reset_pin_monitor.h"
#include "devices/atmega32.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"
#include "devices/enc28j60.h"
#include "utils/fail.h"

using namespace std;

int main(int argc, char **argv)
{
    try {
        if (argc < 3) {
            printf("Invocation: %s <program.elf> <sdcard.bin>\n", argv[0]);
            exit(EXIT_SUCCESS);
        }

        Atmega32 mcu;
        Ds1307 rtc(0x68);
        SdCard sd_card(argv[2], 256*1024*1024);
        Enc28J60 enc28j60;
        I2cBus i2c_bus;
        SpiBus spi_bus;
        
        rtc.connectToI2cBus(&i2c_bus);
        mcu.connectToI2cBus(&i2c_bus);
        
        mcu.connectToSpiBus(&spi_bus);
        sd_card.connectToSpiBus(&spi_bus);
        mcu.addPinMonitor(MEGA32_PIN_B+1, new SlaveSelectPinMonitor(&sd_card, true));
        enc28j60.connectToSpiBus(&spi_bus);
        mcu.addPinMonitor(MEGA32_PIN_B+3, new ResetPinMonitor(&enc28j60, false));
        mcu.addPinMonitor(MEGA32_PIN_B+4, new SlaveSelectPinMonitor(&enc28j60, true));

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
