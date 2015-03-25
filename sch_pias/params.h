#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <linux/types.h>

/* Our module has at most 8 priority queues */
#define PIAS_QDISC_MAX_PRIO 8

/* MTU(1500)+Ethernet header(14) */
#define PIAS_QDISC_MTU_BYTES 1514

/* Ethernet packets with less than the minimum 64 bytes (header (14B) + user data + FCS (4B)) are padded to 64 bytes. */ 
#define PIAS_QDISC_MIN_PKT_BYTES 64	

/* Per-port ECN marking threshold in bytes */
extern int PIAS_QDISC_ECN_THRESH_BYTES;
/* Link speed in Mbps */
extern int PIAS_QDISC_RATE_MBPS;
/* Maximum sum of queue lenghs. It's similar to shared buffer per port on switches */
extern int PIAS_QDISC_MAX_LEN_BYTES;
/* Bucket size in nanosecond */
extern int PIAS_QDISC_BUCKET_NS; 

/* DSCP value for different priority queues (PIAS_QDISC_PRIO_DSCP_1 is for the highest priority) */
extern int PIAS_QDISC_PRIO_DSCP_1;
extern int PIAS_QDISC_PRIO_DSCP_2;
extern int PIAS_QDISC_PRIO_DSCP_3;
extern int PIAS_QDISC_PRIO_DSCP_4;
extern int PIAS_QDISC_PRIO_DSCP_5;
extern int PIAS_QDISC_PRIO_DSCP_6;
extern int PIAS_QDISC_PRIO_DSCP_7;
extern int PIAS_QDISC_PRIO_DSCP_8;

struct PIAS_QDISC_Param {
	char name[64];
	int *ptr;
};

extern struct PIAS_QDISC_Param PIAS_QDISC_Params[16];

/* Intialize parameters and register sysctl */
int pias_qdisc_params_init(void);
/* Unregister sysctl */
void pias_qdisc_params_exit(void);

#endif