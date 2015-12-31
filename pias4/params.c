#include <linux/sysctl.h>
#include <linux/string.h>

#include "params.h"

//Idle time in us
int PIAS_IDLE_TIME = 500;
//RTOmin in us
int PIAS_RTO_MIN = 9 * 1000;
//Threshold of consecutive TCP timeouts to reset priority
int PIAS_TIMEOUT_THRESH = 3;
//Sequence gap in bytes to identify consecutive TCP timeouts
int PIAS_TIMEOUT_SEQ_GAP = 3 * 1448;
//Shall we enable aging to prevent long-term starvation
int PIAS_ENABLE_AGING = 1;
//Shall we enable debug mode
int PIAS_DEBUG_MODE = 0;

int PIAS_PRIO_DSCP[PIAS_PRIO_NUM] = {7, 6, 5, 4, 3, 2, 1, 0};
//int PIAS_PRIO_THRESH[PIAS_PRIO_NUM - 1] = {909*1460, 1329*1460, 1648*1460, 1960*1460, 2143*1460, 2337*1460, 2484*1460};
int PIAS_PRIO_THRESH[PIAS_PRIO_NUM - 1] = {745*1460, 1083*1460, 1391*1460, 13689*1460, 14396*1460, 21149*1460, 27245*1460};

struct PIAS_Param PIAS_Params[2 * PIAS_PRIO_NUM + 6] =
{
	{"idle_time", &PIAS_IDLE_TIME},
	{"rto_min", &PIAS_RTO_MIN},
	{"timeout_thresh", &PIAS_TIMEOUT_THRESH},
	{"timeout_seq_gap", &PIAS_TIMEOUT_SEQ_GAP},
	{"enable_aging", &PIAS_ENABLE_AGING},
	{"debug_mode", &PIAS_DEBUG_MODE}
};

struct ctl_table PIAS_Params_Table[2 * PIAS_PRIO_NUM + 6];

struct ctl_path PIAS_Params_Path[] =
{
	{ .procname = "pias" },
	{ },
};

struct ctl_table_header *PIAS_Sysctl = NULL;

/* Intialize parameters and register sysctl */
bool PIAS_Params_Init(void)
{
    int i = 0;
    struct ctl_table *entry = NULL;

	memset(PIAS_Params_Table, 0, sizeof(PIAS_Params_Table));

    /* Initialize PIAS_Params */
    for (i = 0; i < PIAS_PRIO_NUM; i++)
    {
        snprintf(PIAS_Params[i + 6].name, 63, "prio_dscp_%d", i);
        PIAS_Params[i + 6].ptr = &PIAS_PRIO_DSCP[i];

		/* we have PIAS_PRIO_NUM - 1 demotion thresholds */
		if (i < PIAS_PRIO_NUM - 1)
        {
			snprintf(PIAS_Params[i + PIAS_PRIO_NUM + 6].name, 63, "prio_thresh_%d", i);
        	PIAS_Params[i + PIAS_PRIO_NUM + 6].ptr = &PIAS_PRIO_THRESH[i];
		}
    }
    /* End of the parameters */
    PIAS_Params[2 * PIAS_PRIO_NUM + 6 - 1].ptr = NULL;

    for (i = 0; i < 2 * PIAS_PRIO_NUM + 6; i++)
    {
		if (PIAS_Params[i].ptr)
        {
			entry = &PIAS_Params_Table[i];
        	/* Initialize entry (ctl_table) */
        	entry->procname = PIAS_Params[i].name;
        	entry->data = PIAS_Params[i].ptr;
        	entry->mode = 0644;
        	entry->proc_handler = &proc_dointvec;
        	entry->maxlen = sizeof(int);
		}
    }

    PIAS_Sysctl = register_sysctl_paths(PIAS_Params_Path, PIAS_Params_Table);

    if (PIAS_Sysctl)
        return true;
    else
        return false;
}

/* Unregister sysctl */
void PIAS_Params_Exit(void)
{
    if (PIAS_Sysctl)
        unregister_sysctl_table(PIAS_Sysctl);
}
