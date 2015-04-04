#include "params.h"
#include <linux/sysctl.h>
#include <linux/string.h>

int PIAS_RTO_MIN=8*1000;
int PIAS_TIMEOUT_THRESH=2;
int PIAS_SEQ_GAP_THRESH=3*1448;
 
int PIAS_PRIO_DSCP_1=7;
int PIAS_PRIO_DSCP_2=6;
int PIAS_PRIO_DSCP_3=5;
int PIAS_PRIO_DSCP_4=4;
int PIAS_PRIO_DSCP_5=3;
int PIAS_PRIO_DSCP_6=2;
int PIAS_PRIO_DSCP_7=1;
int PIAS_PRIO_DSCP_8=0;

int PIAS_PRIO_THRESH_1=20*1024;
int PIAS_PRIO_THRESH_2=2147483647;
int PIAS_PRIO_THRESH_3=2147483647;
int PIAS_PRIO_THRESH_4=2147483647;
int PIAS_PRIO_THRESH_5=2147483647;
int PIAS_PRIO_THRESH_6=2147483647;
int PIAS_PRIO_THRESH_7=2147483647;

/* All parameters that can be configured through sysctl */
struct PIAS_param PIAS_params[32]={
	{"PIAS_RTO_MIN\0", &PIAS_RTO_MIN},
	{"PIAS_TIMEOUT_THRESH\0", &PIAS_TIMEOUT_THRESH},
	{"PIAS_SEQ_GAP_THRESH\0", &PIAS_SEQ_GAP_THRESH},
	{"PIAS_PRIO_DSCP_1\0", &PIAS_PRIO_DSCP_1},
	{"PIAS_PRIO_DSCP_2\0", &PIAS_PRIO_DSCP_2},
	{"PIAS_PRIO_DSCP_3\0", &PIAS_PRIO_DSCP_3},
	{"PIAS_PRIO_DSCP_4\0", &PIAS_PRIO_DSCP_4},
	{"PIAS_PRIO_DSCP_5\0", &PIAS_PRIO_DSCP_5},
	{"PIAS_PRIO_DSCP_6\0", &PIAS_PRIO_DSCP_6},
	{"PIAS_PRIO_DSCP_7\0", &PIAS_PRIO_DSCP_7},
	{"PIAS_PRIO_DSCP_8\0", &PIAS_PRIO_DSCP_8},
	{"PIAS_PRIO_THRESH_1\0", &PIAS_PRIO_THRESH_1},
	{"PIAS_PRIO_THRESH_2\0", &PIAS_PRIO_THRESH_2},
	{"PIAS_PRIO_THRESH_3\0", &PIAS_PRIO_THRESH_3},
	{"PIAS_PRIO_THRESH_4\0", &PIAS_PRIO_THRESH_4},
	{"PIAS_PRIO_THRESH_5\0", &PIAS_PRIO_THRESH_5},
	{"PIAS_PRIO_THRESH_6\0", &PIAS_PRIO_THRESH_6},
	{"PIAS_PRIO_THRESH_7\0", &PIAS_PRIO_THRESH_7},
	{"\0", NULL},
};

struct ctl_table PIAS_params_table[32];
struct ctl_path PIAS_params_path[] = {
	{ .procname = "pias" },
	{ },
};
struct ctl_table_header *PIAS_sysctl=NULL;

int PIAS_params_init(void)
{
	int i=0;
	memset(PIAS_params_table, 0, sizeof(PIAS_params_table));
	
	for(i = 0; i < 32; i++) 
	{
		struct ctl_table *entry = &PIAS_params_table[i];
		//End
		if(PIAS_params[i].ptr == NULL)
			break;
		//Initialize entry (ctl_table)
		entry->procname=PIAS_params[i].name;
		entry->data=PIAS_params[i].ptr;
		entry->mode=0644;
		entry->proc_handler=&proc_dointvec;
		entry->maxlen=sizeof(int);
	}
	
	PIAS_sysctl=register_sysctl_paths(PIAS_params_path, PIAS_params_table);
	if(PIAS_sysctl==NULL)
		return -1;
	else	
		return 0;
}

void PIAS_params_exit(void)
{
	if(PIAS_sysctl!=NULL)
		unregister_sysctl_table(PIAS_sysctl);
}
