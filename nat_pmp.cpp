#include "nat_pmp.h"
#include <iostream>

using namespace std;
// create the NAT-PMP'er
// should it forward UDP
// how long should the port be forwarded for
nat_pmp::nat_pmp(bool UDPForward , unsigned int LifeTime)
{
    // set the options, udp forward mode and lifetime of the forward, ie forwarding expires after this time in secs
    this->UDPForward = UDPForward;
    this->PortLifeTime = LifeTime;

    // set up the socket to listen for a response from the NAT-PMP server
    int PMP_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // the endpoint to bind to (0.0.0.0:xxxxx)
    sockaddr_in local_addr;

    local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = INADDR_ANY;
	local_addr.sin_port = 0;

	unsigned int szLocalAddr = sizeof(local_addr);

	//bind for responses
	bind(PMP_socket, (sockaddr*)&local_addr, szLocalAddr);
	getsockname(PMP_socket, (sockaddr*)&local_addr, &szLocalAddr);


	//set the timeout, socket will die if nothing for this long
	/*
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = SOCKET_TIMEOUT;
		if (setsockopt(PMP_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error Setting options on the socket");
	}
    */
	this->PMP_socket = PMP_socket;

	// get the ephemeral port assigned to us
	unsigned short responsePort = ntohs(local_addr.sin_port);

	cout <<  "NAT-PMP response port: " << responsePort << endl;
}

// the DESTRUKTOR!!!
nat_pmp::~nat_pmp()
{
    // close the port bound to.
    if (this->PMP_socket)
    {
        close(this->PMP_socket);

    }
}

//send out the port map request, wait for a response
//if it is successful, return 0, else return an error code
int nat_pmp::map_port(uint16_t internal, uint16_t external)
{
    int status = 0;

    //we need to send to the gateway. but we don't know it. get it
    char gateway[16] = {0};//new char[16];

	const char* get_gateway_cmd = "route -n | grep 'UG[ \t]' | awk '{print $2}' | head -n1";

	FILE* fp = popen(get_gateway_cmd, "r");
	if (!fgets(gateway,sizeof(gateway) -1,fp))
    {
        return -1;
        //an error occurred
    }

    for (unsigned int i = 0; i < sizeof(gateway); i++)
    {
        if (gateway[i] == '\n')
        {
            gateway[i] = 0;
            break;
        }
    }


	// address to send the NAT-PMP packet
    sockaddr_in gateway_addr;
    gateway_addr.sin_family = AF_INET;

	//add ip to the socket
	inet_pton(AF_INET, gateway, &(gateway_addr.sin_addr));

	cout << "Gateway: " << gateway << endl;

    //delete gateway;

    // add the port to the socket
    gateway_addr.sin_port = htons(NAT_PMP_PORT);

    // craft the request packet

    RequestPacket* request = new RequestPacket();

    request->version = 0;
    if (this->UDPForward)
    {
        request->opcode = UDP_REQUEST_MAP_OPCODE;
    }
    else
    {
        request->opcode = TCP_REQUEST_MAP_OPCODE;
    }

    request->reserved = 0;

    request->internalPort = htons(internal);
    request->externalPort = htons(external);

    request->lifetime = htonl(this->PortLifeTime);


    //1st byte = version (0 for now, 2 when i upgrade to pcp)
    //2nd byte = opcode, 1 for udp map or 2 for tcp map
    //bytes 3-4 = reserved (always 0)
    //bytes 5-6 = internal port
    //bytes 7-8 = external port
    //remaining bytes = the lifetime (32 bits)

    // send the port map request
    status = sendto(this->PMP_socket, request, sizeof(RequestPacket), 0, \
    		(sockaddr *)&gateway_addr, sizeof(gateway_addr));
    delete request;

    if (status < 0)
    {
        perror("Error");
        return status;
    }


    // the endpoint of the device sending a response
    sockaddr_in response_addr;
    unsigned int szSOCK_ADDR = sizeof(sockaddr_in);

    // get the response
    uint8_t rawResponse[16];
    status = recvfrom(this->PMP_socket, rawResponse, sizeof(rawResponse), 0, (sockaddr *) &response_addr, &szSOCK_ADDR);


    cout << "Response from " << inet_ntoa(response_addr.sin_addr) << ": " << endl;

    if (status < 0)
    {
        perror("Error");
        return status;
    }

    // parse the response (put it into a nice struct)
    ResponsePacket response;

    response.opcode = rawResponse[1];
    switch (response.opcode)
    {
    case 130:
        cout << "    Map TCP Response (" << to_string(response.opcode) << ")" << endl;
        break;
    case 129:
        cout << "    Map UDP Response (" << to_string(response.opcode) << ")" << endl;
        break;
    default:
        cout << "    Unknown opcode response" << endl;
        status = 8;
        break;
    }

    // the status field
    memcpy(&response.resultCode, rawResponse + 2, 2);
    response.resultCode = ntohs(response.resultCode);
    status = (int)response.resultCode;

    switch (status)
    {
    case 0:
        cout << "    Success (";
        break;
    case 2:
        cout << "    Not Authorized/Refused (";
        break;
    default:
        cout << "    Failure (";
        break;
    }
    cout << to_string(status) << ")" << endl;



    response.version = rawResponse[0];
    cout << "    Protocol version: " << to_string(response.version) << endl;




    memcpy(&response.epoch, rawResponse + 4, 4);
    response.epoch = ntohl(response.epoch);
    cout << "    Seconds Since Start of Epoch: " << to_string(response.epoch) << endl;


    memcpy(&response.internalPort, rawResponse + 8, 2);
    response.internalPort = ntohs(response.internalPort);


    memcpy(&response.externalPort, rawResponse + 10, 2);
    response.externalPort = ntohs(response.externalPort);

    cout << "    Map " << to_string(response.internalPort) << "(internal) to " << to_string(response.externalPort) << "(external)" << endl;

    memcpy(&response.lifetime, rawResponse + 12, 4);
    response.lifetime = ntohl(response.lifetime);
    cout << "    Mapping lasts for " << to_string(response.lifetime) << " seconds" << endl;

    return status;
}
