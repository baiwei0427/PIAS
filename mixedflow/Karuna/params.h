#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

//Hash range
extern const unsigned int Karuna_HASH_RANGE;
//Maximum size of a list
extern const unsigned int Karuna_LIST_SIZE;

extern const u32 Karuna_MAX_FLOW_SIZE;
//RTOmin in us
extern const unsigned int Karuna_RTO_MIN;
extern const unsigned int Karuna_TIMEOUT_THRESH;

//Thresholds for information-aware non-deadline (type 2) flows (Karuna_AWARE_THRESHOLD_1 is the smallest value)
extern const u32 Karuna_AWARE_THRESHOLD_1;
extern const u32 Karuna_AWARE_THRESHOLD_2;
extern const u32 Karuna_AWARE_THRESHOLD_3;
extern const u32 Karuna_AWARE_THRESHOLD_4;
extern const u32 Karuna_AWARE_THRESHOLD_5;
extern const u32 Karuna_AWARE_THRESHOLD_6;
//extern const u32 Karuna_AWARE_THRESHOLD_7;

//Thresholds for information-agnostic non-deadline (type 3) flows (Karuna_AGNOSTIC_THRESHOLD_1 is the smallest value)
extern const u32 Karuna_AGNOSTIC_THRESHOLD_1;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_2;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_3;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_4;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_5;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_6;
//extern const u32 Karuna_AGNOSTIC_THRESHOLD_7;

//DSCP value for different priority queues (PRIORITY_DSCP_1 is for the highest priority)
extern const u8 PRIORITY_DSCP_1;
extern const u8 PRIORITY_DSCP_2;
extern const u8 PRIORITY_DSCP_3;
extern const u8 PRIORITY_DSCP_4;
extern const u8 PRIORITY_DSCP_5;
extern const u8 PRIORITY_DSCP_6;
extern const u8 PRIORITY_DSCP_7;
extern const u8 PRIORITY_DSCP_8;

#endif