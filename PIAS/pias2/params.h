#ifndef __PARAMS_H__
#define __PARAMS_H__

//Maximum size of bytes sent
const unsigned int MAX_BYTES_SENT=4294967295;
//RTOmin is 16ms
const unsigned int RTO_MIN=16*1000;
//The threshold of consecutive TCP timeouts
const unsigned int TIMEOUT_THRESH=3;

#endif 
