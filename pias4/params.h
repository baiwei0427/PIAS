#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

#define PIAS_PRIO_NUM 8 //Number of PIAS priorities

//Idle time in us
extern int PIAS_IDLE_TIME;
//RTOmin in us
extern int PIAS_RTO_MIN;
//Threshold of consecutive TCP timeouts to reset priority
extern int PIAS_TIMEOUT_THRESH;
//Sequence gap in bytes to identify consecutive TCP timeouts
extern int PIAS_TIMEOUT_SEQ_GAP;
//Shall we enable aging to prevent long-term starvation
extern int PIAS_ENABLE_AGING;
//Shall we enable debug mode
extern int PIAS_DEBUG_MODE;

//DSCP value for different priorities
extern int PIAS_PRIO_DSCP[PIAS_PRIO_NUM];
//Demotion thresholds in bytes for different priorities
extern int PIAS_PRIO_THRESH[PIAS_PRIO_NUM - 1];

struct PIAS_Param
 {
	char name[64];
	int *ptr;
};

extern struct PIAS_Param PIAS_Params[2 * PIAS_PRIO_NUM + 6];

/* Intialize parameters and register sysctl. Return true if it succeeds. */
bool PIAS_Params_Init(void);
/* Unregister sysctl */
void PIAS_Params_Exit(void);

#endif
