#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

//Hash range
extern const unsigned int PIAS_HASH_RANGE;
//Maximum size of a list
extern const unsigned int PIAS_LIST_SIZE;

extern const u32 PIAS_MAX_FLOW_SIZE;
//RTOmin in us
extern const unsigned int PIAS_RTO_MIN;
extern const unsigned int PIAS_TIMEOUT_THRESH;

//Sequence gap in bytes
extern const unsigned int PIAS_SEQ_GAP_THRESH;

//7 demotion thresholds
extern const u32 PIAS_THRESHOLD_1;
extern const u32 PIAS_THRESHOLD_2;
extern const u32 PIAS_THRESHOLD_3;
extern const u32 PIAS_THRESHOLD_4;
extern const u32 PIAS_THRESHOLD_5;
extern const u32 PIAS_THRESHOLD_6;
extern const u32 PIAS_THRESHOLD_7;

//DSCP value for different priority queues (PRIORITY_DSCP_1 is for the highest priority)
extern const u8 PIAS_PRIORITY_DSCP_1;
extern const u8 PIAS_PRIORITY_DSCP_2;
extern const u8 PIAS_PRIORITY_DSCP_3;
extern const u8 PIAS_PRIORITY_DSCP_4;
extern const u8 PIAS_PRIORITY_DSCP_5;
extern const u8 PIAS_PRIORITY_DSCP_6;
extern const u8 PIAS_PRIORITY_DSCP_7;
extern const u8 PIAS_PRIORITY_DSCP_8;

#endif