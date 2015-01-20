#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

extern const u32 Karuna_MAX_FLOW_SIZE;
extern const unsigned int Karuna_RTO_MIN;
extern const unsigned int Karuna_TIMEOUT_THRESH;

//Thresholds for information-aware non-deadline flows
extern const u32 Karuna_AWARE_THRESHOLD_1;
extern const u32 Karuna_AWARE_THRESHOLD_2;
extern const u32 Karuna_AWARE_THRESHOLD_3;
extern const u32 Karuna_AWARE_THRESHOLD_4;
extern const u32 Karuna_AWARE_THRESHOLD_5;
extern const u32 Karuna_AWARE_THRESHOLD_6;
extern const u32 Karuna_AWARE_THRESHOLD_7;

//Thresholds for information-agnostic non-deadline flows
extern const u32 Karuna_AGNOSTIC_THRESHOLD_1;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_2;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_3;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_4;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_5;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_6;
extern const u32 Karuna_AGNOSTIC_THRESHOLD_7;


#endif