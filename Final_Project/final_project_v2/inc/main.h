#ifndef MAIN_H
#define MAIN_H

/*@Author       : Om Raheja
 *@File Name    : main.h
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: This file includes all the standard c library header files.
 * */


/* Standard C library Headers */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <linux/videodev2.h>
#include <sched.h>
#include <time.h>
#include <sys/utsname.h>


/* Function prototypes */
/*@Function Name: print_sequencer_analysis
 *@Brief        : This functions logs the start_time, end_time, execution_time and jitter
 		: for sequencer.
 *@Param in     : void
 *@Return       : void
 * */
void print_sequencer_analysis();


/*@Function Name: print_image_capture_analysis
 *@Brief        : This functions logs the start_time, end_time, execution_time and jitter
                : for image capture service.
 *@Param in     : void
 *@Return       : void
 * */
void print_image_capture_analysis();

/*@Function Name: print_image_dump_analysis
 *@Brief        : This functions logs the start_time, end_time, execution_time and jitter
                : for image dump service.
 *@Param in     : void
 *@Return       : void
 * */
void print_image_dump_analysis();

#endif
