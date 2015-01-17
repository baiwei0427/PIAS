#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

extern const unsigned int MCP_ETHERNET_PKT_LEN;
extern const unsigned int MCP_TCP_PAYLOAD_LEN; 
//Size (bytes) ofr Maximum Segment Size
extern const unsigned int MCP_TCP_MSS;

//Initialized RTT value (us)
extern const u32 MCP_INIT_RTT;
//Initialized/Minimum TCP window size (MSS)
extern const unsigned int MCP_INIT_WIN;

//Hash range
extern const unsigned int MCP_HASH_RANGE;
//Maximum size of a list
extern const unsigned int MCP_LIST_SIZE;

//parameter to smooth RTT: srtt=rtt_smooth/1000*srtt+(1000-rtt_smooth)/1000*rtt
extern const unsigned int MCP_RTT_SMOOTH;


#endif