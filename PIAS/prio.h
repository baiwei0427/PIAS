#ifndef	PRIO_H
#define	PRIO_H

//Input: bytes sent (Note that size<0 denotes that no such flow entry or last few packets)
//Output: DSCP
static int priority(unsigned int size)
{
	if(size<=965816)
		return 1;
	else
		return 0;
}
#endif