#include <linux/module.h>
#include <linux/kernel.h>

#include "netfilter.h"
#include "jprobe.h"
#include "params.h"
#include "flow.h"

/* Flow Table */
struct PIAS_Flow_Table ft;

/*
 * The following two functions are related to param_table_operation
 * To clear flow table: echo -n clear > /sys/module/pias/parameters/param_table_operation
 * To print flow table: echo -n print > /sys/module/pias/parameters/param_table_operation
 */
static int pias_set_operation(const char *val, struct kernel_param *kp);
static int pias_noget(const char *val, struct kernel_param *kp);
module_param_call(param_table_operation, pias_set_operation, pias_noget, NULL, S_IWUSR); //Write permission by owner

/* param_dev: NIC to operate PIAS */
char *param_dev = NULL;
MODULE_PARM_DESC(param_dev, "Interface to operate PIAS (NULL=all)");
module_param(param_dev, charp, 0);

int param_port __read_mostly = 0;
MODULE_PARM_DESC(param_port, "Port to match (0=all)");
module_param(param_port, int, 0);

static int pias_set_operation(const char *val, struct kernel_param *kp)
{
	//For debug
	//printk(KERN_INFO "PIAS: param_table_operation is set\n");
	//Clear flow table
	if (strncmp(val, "clear", 5) == 0)
	{
		printk(KERN_INFO "PIAS: clear flow table\n");
		PIAS_Clear_Table(&ft);
	}
	//Print flow table
	else if (strncmp(val, "print", 5) == 0)
	{
		printk(KERN_INFO "PIAS: print flow table\n");
		PIAS_Print_Table(&ft);
	}
	else
		printk(KERN_INFO "PIAS: unrecognized flow table operation\n");

	return 0;
}

static int pias_noget(const char *val, struct kernel_param *kp)
{
	return 0;
}


static int pias_module_init(void)
{
	int i = 0;

    //Get interface
    if (param_dev)
	{
		// trim
		for (i = 0; i < 32 && param_dev[i] != '\0'; i++)
    	{
			if(param_dev[i] == '\n')
        	{
				param_dev[i] = '\0';
				break;
			}
		}
	}

	//Initialize FlowTable
	PIAS_Init_Table(&ft);

	if (PIAS_Params_Init() && PIAS_Netfilter_Init() && PIAS_JProbe_Init())
	{
		printk(KERN_INFO "PIAS: start on %s (TCP port %d)\n", param_dev? param_dev:"any interface", param_port);
		return 0;
	}
	else
		return -1;
}

static void pias_module_exit(void)
{
	PIAS_JProbe_Exit();
	PIAS_Netfilter_Exit();
	PIAS_Params_Exit();
	PIAS_Exit_Table(&ft);

	printk(KERN_INFO "PIAS: stop working\n");
}

module_init(pias_module_init);
module_exit(pias_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAI Wei baiwei0427@gmail.com");
MODULE_VERSION("1.3");
MODULE_DESCRIPTION("Linux kernel module for PIAS (Practical Information-Agnostic flow Scheduling)");
