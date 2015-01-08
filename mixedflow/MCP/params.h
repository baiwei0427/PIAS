#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

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


#endif