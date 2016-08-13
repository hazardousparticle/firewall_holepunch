#ifndef NAT_PMP_H
#define NAT_PMP_H

#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include <cstring>

//socket listen time out (sec)
#define SOCKET_TIMEOUT 100000

// NAT-PMP constants
#define NAT_PMP_PORT 5351


//NAT-PMP opcodes
#define UDP_REQUEST_MAP_OPCODE 0x01
#define TCP_REQUEST_MAP_OPCODE 0x02


class nat_pmp
{
    public:
        //default time out of 1hr
        nat_pmp(bool UDPForward = false, unsigned int LifeTime = 3600);
        ~nat_pmp();

        int map_port(uint16_t internal, uint16_t external);

    protected:

    private:
        bool UDPForward;
        int PortLifeTime;
        // the socket to handle the UDP communications
        int PMP_socket;
};

#pragma pack(1)
typedef struct
{
    uint8_t version;
    uint8_t opcode;
    uint16_t reserved;
    uint16_t internalPort;
    uint16_t externalPort;
    uint32_t lifetime;

} RequestPacket;
#pragma pack(0)

// the response packet
typedef struct
{
    uint8_t version;
    uint8_t opcode;
    uint16_t resultCode;
    uint32_t epoch;
    uint16_t internalPort;
    uint16_t externalPort;
    uint32_t lifetime;
} ResponsePacket;

#endif // NAT_PMP_H
