#ifndef __PARAMS_H__
#define __PARAMS_H__

//Maximum size of bytes sent
const u32 MAX_BYTES_SENT=4294967295;
//RTOmin is 20ms in our testbed. We set RTO_MIN to 16ms to avoid deviation.
const unsigned int RTO_MIN=16*1000;
//The threshold of consecutive TCP timeouts
const unsigned int TIMEOUT_THRESH=3;

#endif 
