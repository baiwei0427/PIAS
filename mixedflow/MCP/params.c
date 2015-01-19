#include "params.h"

const unsigned int MCP_TCP_PAYLOAD_LEN=1448; 
const unsigned int MCP_TCP_MSS=1460;

const unsigned int MCP_INIT_WIN=3;
const unsigned int MCP_MIN_WIN=2;

const unsigned int MCP_HASH_RANGE=256;
const unsigned int MCP_LIST_SIZE=32;

const u32 MCP_INIT_RTT=100;
const unsigned int MCP_RTT_SMOOTH=875;
const u32 MCP_MAX_RTT=1000;