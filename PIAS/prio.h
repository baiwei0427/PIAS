#ifndef PRIO_H
#define PRIO_H

//Input: bytes sentry
//Output: DSCP
static int priority(int size)
{
	if(size<20*1000)
		return 1;
	else
		return 0;
}
#endif