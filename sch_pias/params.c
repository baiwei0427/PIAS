#include "params.h"
#include <linux/sysctl.h>
#include <linux/string.h>

/* As recommended by DCTCP paper, we use 30KB for 1G network*/
int PIAS_QDISC_ECN_THRESH_BYTES=30*1024;

/* Maximum sum of queue lenghs. 1MB should be enough */ 
int PIAS_QDISC_MAX_LEN_BYTES=1024*1024;

/* Bucket size in nanosecond */
int PIAS_QDISC_BUCKET_NS=2000000; 

/* DSCP values for different priorities */
int PIAS_QDISC_PRIO_DSCP_1=7;
int PIAS_QDISC_PRIO_DSCP_2=6;
int PIAS_QDISC_PRIO_DSCP_3=5;
int PIAS_QDISC_PRIO_DSCP_4=4;
int PIAS_QDISC_PRIO_DSCP_5=3;
int PIAS_QDISC_PRIO_DSCP_6=2;
int PIAS_QDISC_PRIO_DSCP_7=1;
int PIAS_QDISC_PRIO_DSCP_8=0;


/* All parameters that can be configured through sysctl */
struct PIAS_QDISC_Param PIAS_QDISC_Params[16]={
	{"PIAS_QDISC_ECN_THRESH_BYTES\0", &PIAS_QDISC_ECN_THRESH_BYTES},
	{"PIAS_QDISC_MAX_LEN_BYTES\0", &PIAS_QDISC_MAX_LEN_BYTES},
	{"PIAS_QDISC_BUCKET_NS\0",&PIAS_QDISC_BUCKET_NS},
	{"PIAS_QDISC_PRIO_DSCP_1\0", &PIAS_QDISC_PRIO_DSCP_1},
	{"PIAS_QDISC_PRIO_DSCP_2\0", &PIAS_QDISC_PRIO_DSCP_2},
	{"PIAS_QDISC_PRIO_DSCP_3\0", &PIAS_QDISC_PRIO_DSCP_3},
	{"PIAS_QDISC_PRIO_DSCP_4\0", &PIAS_QDISC_PRIO_DSCP_4},
	{"PIAS_QDISC_PRIO_DSCP_5\0", &PIAS_QDISC_PRIO_DSCP_5},
	{"PIAS_QDISC_PRIO_DSCP_6\0", &PIAS_QDISC_PRIO_DSCP_6},
	{"PIAS_QDISC_PRIO_DSCP_7\0", &PIAS_QDISC_PRIO_DSCP_7},
	{"PIAS_QDISC_PRIO_DSCP_8\0", &PIAS_QDISC_PRIO_DSCP_8},
	{"\0", NULL},
};

struct ctl_table PIAS_QDISC_Params_Table[16];
struct ctl_path PIAS_QDISC_Params_Path[] = {
	{ .procname = "sch_pias" },
	{ },
};
struct ctl_table_header *PIAS_QDISC_Sysctl=NULL;

int pias_qdisc_params_init(void)
{
	int i=0;
	memset(PIAS_QDISC_Params_Table, 0, sizeof(PIAS_QDISC_Params_Table));
	
	for(i = 0; i < 16; i++) 
	{
		struct ctl_table *entry = &PIAS_QDISC_Params_Table[i];
		//End
		if(PIAS_QDISC_Params[i].ptr == NULL)
			break;
		//Initialize entry (ctl_table)
		entry->procname=PIAS_QDISC_Params[i].name;
		entry->data=PIAS_QDISC_Params[i].ptr;
		entry->mode=0644;
		entry->proc_handler=&proc_dointvec;
		entry->maxlen=sizeof(int);
	}
	
	PIAS_QDISC_Sysctl=register_sysctl_paths(PIAS_QDISC_Params_Path, PIAS_QDISC_Params_Table);
	if(PIAS_QDISC_Sysctl==NULL)
		return -1;
	else	
		return 0;
}

void pias_qdisc_params_exit(void)
{
	if(PIAS_QDISC_Sysctl!=NULL)
		unregister_sysctl_table(PIAS_QDISC_Sysctl);
}
