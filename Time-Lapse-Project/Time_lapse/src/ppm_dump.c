/*@Author       : Om Raheja
 *@File Name    : ppm_dump.c
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Converts the obtained data in bugbuffer into ppm files.
 */


/* User defined header files */
#include "../inc/ppm_dump.h"


/* Standard C library headers */
#include <pthread.h>
#include <syslog.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>


/* External global variables */
extern struct timespec frame_time;	//For timestamp
extern unsigned int  framecnt;		//Store frame number for ppm header
extern int size_of_image;		//To store size of image for use in different functions
extern unsigned char bigbuffer[(1280*960*3)];
extern sem_t semS2;
extern int abortS2;

/* PPM header */
extern char ppm_header[50];
extern char ppm_dumpname[];

/* Structures */
typedef struct
{
	int threadIdx;
	int MajorPeriods;
	int frame_count;
	unsigned long long sequencePeriods;
} threadParams_t;


void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
	int written, i, total, dumpfd;
	snprintf(&ppm_dumpname[4], 9, "%08d", tag);
	strncat(&ppm_dumpname[12], ".ppm", 5);
	dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
	snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
	strncat(&ppm_header[14], " sec ", 5);
	snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
	//strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);
	written=write(dumpfd, ppm_header, sizeof(ppm_header));
	total=0;
	do
	{
		written=write(dumpfd, p, size);
		total+=written;
	} while(total < size);
	//printf("wrote %d bytes\n", total);
	close(dumpfd);
}


void *Service_2(void *threadp)
{
	struct timeval current_time_val;
	double current_time;
	unsigned long long S2Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	struct timespec S2_start_time;
	struct timespec S2_end_time;

	while(!abortS2)
	{
		sem_wait(&semS2);
		syslog(LOG_INFO,"IMAGE DUMP");
		clock_gettime(CLOCK_REALTIME, &S2_start_time);
		syslog(LOG_INFO,"S2 Count: %lld\t Image Dump start Time: %lf seconds\n",S2Cnt,((double)S2_start_time.tv_sec + (double)((S2_start_time.tv_nsec)/(double)1000000000)));
		S2Cnt++;
		framecnt++;
		dump_ppm(bigbuffer, ((size_of_image*6)/4), framecnt, &frame_time);
		clock_gettime(CLOCK_REALTIME, &S2_end_time);
		syslog(LOG_INFO,"S2 Count: %lld\t Image Dump End Time: %lf seconds\n",S2Cnt,((double)S2_end_time.tv_sec + (double)((S2_end_time.tv_nsec)/(double)1000000000)));
	}

	pthread_exit((void *)0);
}
