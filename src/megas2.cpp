#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <time.h>

#include "glue/i2c_bus.h"
#include "glue/spi_bus.h"
#include "glue/ss_pin_monitor.h"
#include "glue/reset_pin_monitor.h"
#include "devices/atmega32/atmega32.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"
#include "devices/enc28j60.h"
#include "utils/fail.h"
#include "simulation/simulation.h"

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

using namespace std;

class Console : public SimulatedDevice, public PinMonitor
{
public:
    Console(SDL_Surface *screen)
    {
        this->screen = screen;
        this->next_frame_time = 0;
    }
    
    virtual void setSimulationTime(sim_time_t time)
    {
        sim_time_t delta = time - this->sim_time;
        
        this->sim_time = time;
        this->next_frame_time += delta;
    }

    virtual void act()
    {
        this->sim_time = this->next_frame_time;
        
        SDL_Event event;
        
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                exit(EXIT_SUCCESS);
        }
        
        boxColor(screen, 0, 0, 640, 480, 0x000000ff);
        boxColor(screen, 16, 16, 32, 32, this->debug_led_lit ? 0xffc000ff : 0x806000ff);
        
        SDL_Flip(screen);
        
        this->next_frame_time = this->next_frame_time + ms_to_sim_time(20);
    }

    virtual sim_time_t nextEventTime()
    {
        return this->next_frame_time;
    }

    virtual void onPinChanged(int pin, int value)
    {
        debug_led_lit = value;
    }
private:
    sim_time_t next_frame_time;
    
    SDL_Surface *screen;
    bool debug_led_lit;
};

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
    mcu.addPinMonitor(MEGA32_PIN_B+1, new SlaveSelectPinMonitor(&sd_card, true));
    enc28j60.connectToSpiBus(&spi_bus);
    mcu.addPinMonitor(MEGA32_PIN_B+3, new ResetPinMonitor(&enc28j60, false));
    mcu.addPinMonitor(MEGA32_PIN_B+4, new SlaveSelectPinMonitor(&enc28j60, true));
    
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

        SDL_Init(SDL_INIT_VIDEO);

        SDL_Surface *screen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
        if (!screen)
            fail("Could not initialize SDL display");
        atexit(SDL_Quit);
        
        SDL_WM_SetCaption("MEGAS-2 CHARLIE simulation", 0);
        
        Console con(screen);
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
        mcu.addPinMonitor(MEGA32_PIN_D+7, &con);

        mcu.loadProgramFromElf(argv[1]);
        
        Simulation sim;
        sim.addDevice(&mcu);
        sim.addDevice(&rtc);
        sim.addDevice(&sd_card);
        sim.addDevice(&enc28j60);
        sim.addDevice(&con);
        
        sim.run();
    } catch (exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
