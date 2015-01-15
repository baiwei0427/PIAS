#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

//MCP protocol number. We don't use TCP (6)
extern const u8 MCP_PROTOCOL;
//Size (bytes) ofr Maximum Segment Size
extern const unsigned int MCP_MSS;
//Initialized RTT value (us)
extern const u32 MCP_INIT_RTT;
//Initialized TCP window size (MSS)
extern const unsigned int MCP_INIT_WIN;
//Hash range
extern const unsigned int MCP_HASH_RANGE;
//Maximum size of a list
extern const unsigned int MCP_LIST_SIZE;
//parameter to smooth RTT: srtt=rtt_smooth/1000*srtt+(1000-rtt_smooth)/1000*rtt
extern const unsigned int MCP_RTT_SMOOTH;


#endif