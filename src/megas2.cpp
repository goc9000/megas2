#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <time.h>

#include "gui/dashboard.h"
#include "gui/led.h"
#include "glue/analog_bus.h"
#include "glue/i2c_bus.h"
#include "glue/spi_bus.h"
#include "devices/atmega32/atmega32.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"
#include "devices/enc28j60.h"
#include "utils/fail.h"
#include "simulation/simulation.h"

using namespace std;

void benchmark(int argc, char **argv)
{
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
    AnalogBus sdcard_ss(&mcu, MEGA32_PIN_B+1, &sd_card, SDCARD_PIN_SLAVE_SELECT);
    enc28j60.connectToSpiBus(&spi_bus);
    AnalogBus encj_reset(&mcu, MEGA32_PIN_B+3, &enc28j60, E28J_PIN_RESET);
    AnalogBus encj_ss(&mcu, MEGA32_PIN_B+4, &enc28j60, E28J_PIN_SLAVE_SELECT);

    mcu.loadProgramFromElf(argv[1]);

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    long long count = 0;
    
    while (true) {
        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        for (int i = 0; i < 1000; i++) {
            mcu.act();
        }
        count += 1000;

        if (((t1.tv_sec - t0.tv_sec)*1000000000LL + t1.tv_nsec - t0.tv_nsec) > 5000000000LL) {
            break;
        }
    }

    printf("%lld instructions/second (%d%%)\n", count/5, (int)(count*100/5/16000000));

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    try {
        if (argc < 3) {
            printf("Invocation: %s <program.elf> <sdcard.bin>\n", argv[0]);
            exit(EXIT_SUCCESS);
        }

        Dashboard dash("charlie_dashboard_bkgd.png", "font.ttf");
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
        AnalogBus sdcard_ss(&mcu, MEGA32_PIN_B+1, &sd_card, SDCARD_PIN_SLAVE_SELECT);
        enc28j60.connectToSpiBus(&spi_bus);
        AnalogBus encj_reset(&mcu, MEGA32_PIN_B+3, &enc28j60, E28J_PIN_RESET);
        AnalogBus encj_ss(&mcu, MEGA32_PIN_B+4, &enc28j60, E28J_PIN_SLAVE_SELECT);
        
        SimpleLed debug_led(16, 16, 16, 0xff0000ff, "Debug LED");
        AnalogBus led_input(&mcu, MEGA32_PIN_D+7, &debug_led, LED_PIN_INPUT);
        dash.addWidget(&debug_led);

        mcu.loadProgramFromElf(argv[1]);
        
        Simulation sim;
        sim.addDevice(&mcu);
        sim.addDevice(&rtc);
        sim.addDevice(&sd_card);
        sim.addDevice(&enc28j60);
        sim.addDevice(&dash);
        
        sim.run();
    } catch (exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
