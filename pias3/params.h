#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

//Hash range (Number of lists)
#define PIAS_HASH_RANGE (256)
//Maximum lengh of a list
#define PIAS_LIST_SIZE (32)
//Maximum flow size (maximum value of signed 32-bit integer due to sysctl)
#define PIAS_MAX_FLOW_SIZE (2147483647)

//RTOmin in us
extern int PIAS_RTO_MIN;
//Threshold of consecutive timeouts to reset priority
extern int PIAS_TIMEOUT_THRESH;
//Sequence gap in bytes
extern int PIAS_SEQ_GAP_THRESH;

//DSCP value for different priority queues (PIAS_PRIO_DSCP_1 is for the highest priority)
extern int PIAS_PRIO_DSCP_1;
extern int PIAS_PRIO_DSCP_2;
extern int PIAS_PRIO_DSCP_3;
extern int PIAS_PRIO_DSCP_4;
extern int PIAS_PRIO_DSCP_5;
extern int PIAS_PRIO_DSCP_6;
extern int PIAS_PRIO_DSCP_7;
extern int PIAS_PRIO_DSCP_8;

//7 demotion thresholds in bytes. PIAS_PRIO_THRESH_1 is the smallest one
extern int PIAS_PRIO_THRESH_1;
extern int PIAS_PRIO_THRESH_2;
extern int PIAS_PRIO_THRESH_3;
extern int PIAS_PRIO_THRESH_4;
extern int PIAS_PRIO_THRESH_5;
extern int PIAS_PRIO_THRESH_6;
extern int PIAS_PRIO_THRESH_7;

struct PIAS_param {
	char name[64];
	int *ptr;
};

extern struct PIAS_param PIAS_params[32];


//Intialize parameters and register sysctl
int PIAS_params_init(void);
//Unregister sysctl
void PIAS_params_exit(void);

#endif