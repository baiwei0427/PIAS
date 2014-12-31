#ifndef	PRIO_H
#define	PRIO_H

//Input: bytes sent (Note that size<0 denotes that no such flow entry or last few packets)
//Output: DSCP
static u8 priority(unsigned int size)
{
	//For Web Search, it's 965816 
	/*if(size<=965816)
		return  1;
	else
		return 0;*/
	//For Data Mining, it's 6KB
	/*if(size<=6*1024)
		return  1;
	else
		return 0;*/
	//For Data Mining with four queues, our thresholds are 1KB, 9KB and 167KB (load=0.8)
	/*
	if(size<=1*1024)
		return 3;
	else if(size<=9*1024)
		return 2;
	else if(size<=167*1024)
		return 1;
	else
		return 0;*/
	//For Data Mining with eight queues, our thresholds are 1KB, 8KB, 80KB, 122KB, 130KB, 132KB, 133KB
	if(size<=1*1024)
		return 7;
	else if(size<=8*1024)
		return 6;
	else if(size<=80*1024)
		return 5;
	else if(size<=122*1024)
		return 4;
	else if(size<=130*1024)
		return 3;
	else if(size<=132*1024)
		return 2;
	else if(size<=133*1024)
		return 1;
	else
		return 0;
	//For Web Search with two queues (equal split), our thresholds are 72KB
	/*if(size<=72*1024)
		return 1;
	else
		return 0;*/
	//For Web Search with four queues (equal split), our thresholds are 24KB, 72KB, 1448KB
	/*if(size<=24*1024)
		return 3;
	else if(size<=72*1024)
		return 2;
	else if(size<=1448*1024)
		return 1;
	else
		return 0;*/
	//For Web Search with four queues (optimal), our thresholds are  70KB   1785KB   3.18MB
	/*if(size<=70*1024)
		return 3;
	else if(size<=1785*1024)
		return 2;
	else if(size<=3180*1024)
		return 1;
	else
		return 0;*/
	//For Web search with eight queues (optimal), our thresholds are  65KB  1491KB  2257KB  2814KB 2880KB 2896KB 3000KB
	/*if(size<=65*1024)
		return 7;
	else if(size<=1491*1024)
		return 6;
	else if(size<=2257*1024)
		return 5;
	else if(size<=2814*1024)
		return 4;
	else if(size<=2880*1024)
		return 3;
	else if(size<=2896*1024)
		return 2;
	else if (size<=3000*1024)
		return 1;
	else 
		return 0;*/
	//For static flow experiment, the right threshold is 20KB 
	/*if(size<=1024*1024)
		return 1;
	else 
		return 0;*/
}
#endif