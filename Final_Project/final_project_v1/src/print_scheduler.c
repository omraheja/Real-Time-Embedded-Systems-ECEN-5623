/*@Author       : Om Raheja
 *@File Name    : print_scheduler.c
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Prints the scheduling policy set.
 */


/* User defined headers */
#include "../inc/print_scheduler.h"

/* Standard C library headers */
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


void print_scheduler(void)
{
	int schedType;

	schedType = sched_getscheduler(getpid());

	switch(schedType)
	{
		case SCHED_FIFO:
			printf("Pthread Policy is SCHED_FIFO\n");
			break;

		case SCHED_OTHER:
			printf("Pthread Policy is SCHED_OTHER\n");
			break;

		case SCHED_RR:
			printf("Pthread Policy is SCHED_RR\n");
			exit(-1);
			break;

		default:
			printf("Pthread Policy is UNKNOWN\n");
			exit(-1);
	}
}
