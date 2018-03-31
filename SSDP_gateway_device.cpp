#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <errno.h>

#include "SSDP_gateway_device.h"

char* gatewayAddress(void)
{
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
    tv.tv_sec = SOCKET_TIMEOUT_S;
    tv.tv_usec = 0;//SOCKET_TIMEOUT_US;
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
    delete[] requestPacket;
    requestPacket = nullptr;

    char RecvBuf[MAX_BUF_LEN];

    sockaddr_in GatewayDevice;

    // stop after so many attempts, if too many attempts have occurred
    // TODO: use the fallback method if this fails
    // fallback method is the route command

    int result = recvfrom(ssdp_sock, RecvBuf, sizeof RecvBuf, 0, (sockaddr *) & GatewayDevice, &szLocalAddr);
    // receive sockets, and put senders IP in buffer

    if (result >= MAX_BUF_LEN)
    {
        result = MAX_BUF_LEN -1;
    }

    if (result < 0)
    {
        // error receiving from sockets
        //perror("Error");
        return nullptr;
    }

    RecvBuf[result] = 0;

    char* response_address = inet_ntoa(GatewayDevice.sin_addr);

    std::string str(RecvBuf);


    // parse the response and check if it is a gateway
    int indexStart = str.find("ST: urn:", 0);

    if (indexStart == std::string::npos)
    {
        // couldn't find ST: in the response.
        return nullptr; //try again
    }

    // check if the ST: urn: is a gateway

    int indexEnd = str.find(":1\r\n", indexStart);
    if (indexStart == std::string::npos)
    {
        return nullptr; //try again
    }

    int fieldSize = indexEnd - (indexStart + 8);

    char* field = new char[fieldSize + 1] {0};

    strncpy(field, RecvBuf + indexStart + 8, fieldSize);
    //field[fieldSize] = 0;


    if (strncmp(field, "schemas-upnp-org:device:InternetGatewayDevice", 45))
    {
        //not a IGD. :(
        delete[] field;
        return nullptr;
    }

    delete[] field;

    // parse to get the location
    indexStart = str.find("LOCATION: http://");

    if (indexStart == std::string::npos)
    {
        // couldn't find the location field in the response.
        return nullptr; //try again
    }

    //extract the IP address from the LOCATION Field
    indexEnd = str.find(":", indexStart + 17);
    if (indexStart == std::string::npos)
    {
        // couldn't parse the location field in the response.
        return nullptr; //try again
    }

    fieldSize = indexEnd - (indexStart + 17);

    field = new char[fieldSize +1] {0};

    strncpy(field, RecvBuf + indexStart + 17, fieldSize);

    if (strcmp(response_address, field))
    {
        //ip from packet doesn't match the ip provided in the response
        delete[] field;
        return nullptr;
    }

    delete[] field;

    // return the gateway
    gateway = response_address;

    // print the type of server
    indexStart = str.find("SERVER: ");
    if (indexStart != std::string::npos)
    {
        indexEnd = str.find("\r\n", indexStart);

        fieldSize = indexEnd - (indexStart + 8);
        field = new char[fieldSize + 1] {0};

        strncpy(field, RecvBuf + indexStart + 8, fieldSize);

        printf("Gateway device: %s\n", field);
        delete[] field;
    }


    field = nullptr;

    close(ssdp_sock);

    return gateway;
}
