#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include <cstring>
#include <errno.h>
#include "stringSearch.h"
#include "SSDP_gateway_device.h"

char* gatewayAddress(void)
{

    // TODO: known issue fix, firewall blocks receiving SSDP messages

    char* gateway = nullptr;

    //SSDP socket
    int ssdp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    //address
    sockaddr_in mcast_addr, local_addr;


    if (ssdp_sock < 0)
    {
        //error making socket
        perror("Error");
        return gateway;
    }

    mcast_addr.sin_family = AF_INET;
    //add ip to the socket
    inet_pton(AF_INET, SSDP_MULTICAST, &(mcast_addr.sin_addr));

    mcast_addr.sin_port = htons(SSDP_PORT);


    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = 0;

    unsigned int szLocalAddr = sizeof(local_addr);


    //bind for responses
    bind(ssdp_sock, (sockaddr*)&local_addr, szLocalAddr);
    getsockname(ssdp_sock, (sockaddr*)&local_addr, &szLocalAddr);


    //set the timeout, socket will die if nothing for this long

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = SOCKET_TIMEOUT;
    if (setsockopt(ssdp_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
        perror("Error");
        return gateway;
    }

    //unsigned short responsePort = ntohs(local_addr.sin_port);

    //prepare the broadcast message

    char *requestPacket = new char[500] {0};

    sprintf(requestPacket, \
            "M-SEARCH * HTTP/1.1\r\nHOST: %s:%i\r\nST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\n", \
            SSDP_MULTICAST, SSDP_PORT);

    //send the message over UDP
    if (sendto(ssdp_sock, (char *)requestPacket, strlen(requestPacket), 0, \
            (sockaddr *)&mcast_addr, sizeof(mcast_addr)) < 0)
    {
        perror("Error");
        return gateway;
        //failed to send
    }
    delete requestPacket;

    char RecvBuf[MAX_BUF_LEN];

    sockaddr_in GatewayDevice;

    // TODO: stop after so many attempts, if too many attempts have occured, use the fallback method
    // fallback method is the route command
    while (true)
    {
        int result = recvfrom(ssdp_sock, RecvBuf, sizeof RecvBuf, 0, (sockaddr *) & GatewayDevice, &szLocalAddr);
        // receive sockets, and put senders IP in buffer

        if (result >= MAX_BUF_LEN)
        {
            result = MAX_BUF_LEN -1;
        }

        if (result < 0)
        {
            // error receiving from sockets
            perror("Error");
            continue;
        }

        RecvBuf[result] = 0;

        char* response_address = inet_ntoa(GatewayDevice.sin_addr);

        // parse the response and check if it is a gateway
        int indexStart = findInString("ST: urn:", RecvBuf, 0);

        if (indexStart < 0)
        {
            // couldn't find the ST: in the response.
            continue; //try again
        }

        // check if the ST: urn: is a gateway

        int indexEnd = findInString(":1\r\n", RecvBuf, indexStart);
        if (indexEnd < 0)
        {
            // couldn't find the ST: in the response.
            continue; //try again
        }

        int fieldSize = indexEnd - (indexStart + 8);

        char* field = new char[fieldSize] {0};

        strncpy(field, RecvBuf + indexStart + 8, fieldSize);
        field[fieldSize] = 0;


        if (strncmp(field, "schemas-upnp-org:device:InternetGatewayDevice", 45))
        {
            //not a IGD. :(
            delete field;
            continue;
        }

        delete field;

        // parse to get the location
        indexStart = findInString("LOCATION: http://", RecvBuf, 0);

        if (indexStart < 0)
        {
            // couldn't find the ST: in the response.
            continue; //try again
        }

        //extract the IP address from the LOCATION Field
        indexEnd = findInString(":", RecvBuf, indexStart + 17);
        if (indexEnd < 0)
        {
            // couldn't find the : in the response.
            continue; //try again
        }

        fieldSize = indexEnd - (indexStart + 17);

        field = new char[fieldSize] {0};
        strncpy(field, RecvBuf + indexStart + 17, fieldSize);
        field[fieldSize] = 0;

        if (strcmp(response_address, field))
        {
            //ip from packet doesn't match the ip provided in the response
            continue;
        }

        delete field;

        // return the gateway
        gateway = response_address;
        break;

    }

    close(ssdp_sock);

    return gateway;
}