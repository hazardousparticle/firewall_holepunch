#include <limits.h>
#include <libgen.h>
//#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "nat_pmp.h"
#include "SSDP_gateway_device.h"


// SSDP retries, if nothing for so many attempts, retry this many times.
// Wait x seconds between each
#define SSDP_SEARCH_RETRY 4
//#define SSDP_DELAY_BETWEEN_RETRIES 1

using namespace std;

void usage(const char* arg0)
{
    char arg[255] = {0};

    strncpy(arg, arg0, sizeof(arg));

    char* filename = basename(arg);

    cout << "Usage: " << endl;
    cout << filename << " [internal TCP Port] [external TCP port]" << endl;
    cout << filename << " -U [internal UDP Port] [external UDP port]" << endl;
    exit(1);
}


int main(int argc, const char * argv[])
{
    uint16_t internal_port = 0;
    uint16_t external_port = 0;
    bool UDP_mode = false;

    //parse arg strings into numbers
    auto StrToUint16 = [](const char * str) -> uint16_t
    {
        char * bad_chars;

        long l = strtol(str, &bad_chars, 0);

        if (strlen(bad_chars) > 0)
        {
            return 0;
        }

        if (l > USHRT_MAX || l < 0)
        {
            l = 0;
        }

        return (uint16_t) l;
    };



    if (argc == 4)
    {
        //handle UDP port map

        if (strncmp(argv[1], "-U", 3))
        {
            // wrong arg
            usage(argv[0]);
        }

        UDP_mode = true;

        internal_port = StrToUint16(argv[2]);
        external_port = StrToUint16(argv[3]);
    }
    else if (argc == 3)
    {
        //handle TCP port map
        internal_port = StrToUint16(argv[1]);
        external_port = StrToUint16(argv[2]);
    }
    else
    {
        //only allow 3/4 args
        usage(argv[0]);
    }

    if (!internal_port || !external_port)
    {
        usage(argv[0]);
    }

    char* gateway = nullptr;

    for (int i = 0; i < SSDP_SEARCH_RETRY; i++)
    {
        gateway = gatewayAddress();
        if (gateway)
        {
            break;
        }
        //sleep(SSDP_DELAY_BETWEEN_RETRIES);
        // already a delay when waiting for a response

        //no answer try again
    }

    if (!gateway)
    {
        // error finding the gateway :(
        cerr << "Error: No Response from gateway device." << endl;
        return -1;
    }

    cout << "Gateway address: " << gateway << endl;

    //numbers are correct do the thing
    nat_pmp *PortMapper;
    if (UDP_mode)
    {
        PortMapper = new nat_pmp(gateway, true);
    }
    else
    {
        PortMapper = new nat_pmp(gateway);
    }

    int result = PortMapper->map_port(internal_port, external_port);
    // clean up
    delete PortMapper;

    return result;
}
