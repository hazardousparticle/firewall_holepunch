#ifndef SSDP_GATEWAY_DEVICE_H_INCLUDED
#define SSDP_GATEWAY_DEVICE_H_INCLUDED

#define SSDP_MULTICAST "239.255.255.250"
#define SSDP_PORT 1900

//time out per received ssdp response
#define SOCKET_TIMEOUT 500000

#define MAX_ATTEMPTS 10
#define MAX_BUF_LEN 500

char* gatewayAddress(void);

#endif // SSDP_GATEWAY_DEVICE_H_INCLUDED
