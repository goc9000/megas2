#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <time.h>

#include "simulation/sys_desc.h"

int main(int argc, char **argv)
{
    try {
        if (argc < 2) {
            printf("Invocation: %s <system.msd>\n", argv[0]);
            
            return EXIT_SUCCESS;
        }

        SystemDescription sysdesc(argv[1]);
    } catch (exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
