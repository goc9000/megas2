#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <time.h>

#include "utils/fail.h"
#include "utils/time.h"
#include "simulation/sys_desc.h"
#include "simulation/simulation.h"

#define BENCHMARK_SECONDS           5

const char *param_sys_desc_file = NULL;
bool param_do_benchmark = false;

void run_benchmark(Simulation *sim)
{
    struct timespec t0, t1;

    sim->sync_with_real_time = false;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    sim->runToTime(sec_to_sim_time(BENCHMARK_SECONDS));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    int64_t sim_elapsed = BENCHMARK_SECONDS * 1000000000LL;
    int64_t real_elapsed = timespec_delta_ns(&t1, &t0);

    printf("Unsynced speeed: %d%%\n", (int)(100LL*sim_elapsed/real_elapsed));

    exit(EXIT_SUCCESS);
}

void show_help()
{
    printf("Invocation: megas2 [--benchmark] <system.msd>\n");
    
    exit(EXIT_SUCCESS);
}

void process_args(int argc, char **argv)
{
    for (int i=1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "--benchmark")) {
                param_do_benchmark = true;
            } else {
                fail("Unknown flag '%s'", argv[i]);
            }
        } else {
            if (param_sys_desc_file == NULL) {
                param_sys_desc_file = argv[i];
            } else {
                fail("Too many command-line arguments");
            }
        }
    }
    
    if (param_sys_desc_file == NULL) {
        show_help();
    }
}

int main(int argc, char **argv)
{
    try {
        process_args(argc, argv);

        SystemDescription sys_desc(param_sys_desc_file);
        Simulation sim(&sys_desc);
        
        if (param_do_benchmark) {
            run_benchmark(&sim);
        } else {
            sim.run();
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
