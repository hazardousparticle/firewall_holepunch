#ifndef SSDP_GATEWAY_DEVICE_H_INCLUDED
#define SSDP_GATEWAY_DEVICE_H_INCLUDED

#define SSDP_MULTICAST "239.255.255.250"
#define SSDP_PORT 1900

//time out per received ssdp response
//microseconds
//#define SOCKET_TIMEOUT_US 500000
//seconds
#define SOCKET_TIMEOUT_S 3

// maximum times the recvfrom should return
// returns on time out or received bytes.
#define MAX_ATTEMPTS 3

// length of the buffer to hold responses from the SSDP requests
#define MAX_BUF_LEN 500

char* gatewayAddress(void);

#endif // SSDP_GATEWAY_DEVICE_H_INCLUDED
