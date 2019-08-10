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
#include <stdlib.h>

/* External global variables */
extern struct timespec frame_time;	//For timestamp
extern unsigned int  framecnt;		//Store frame number for ppm header
extern int size_of_image;		//To store size of image for use in different functions
extern unsigned char bigbuffer[(640*480*3)];
extern sem_t semS2;
extern int abortS2;
extern int abortS3;
extern int abortS4;
extern sem_t semS3;

extern int frame_count;

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






double *execution_time_img_dump; //To store execution time for each iteration
double *start_time_img_dump;     //To store start time for each iteration
double *end_time_img_dump;       //To store end time for each iteration


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
	
	syslog(LOG_INFO,"************************write ppm header*************************");

	written=write(dumpfd, ppm_header, sizeof(ppm_header));
	syslog(LOG_INFO,"*******************ppm header written*******************");

	total=0;

	do
	{
		written=write(dumpfd, p, size);
		total+=written;
	} while(total < size);

	syslog(LOG_INFO,"******************after do while***********************");

	//printf("wrote %d bytes\n", total);
	close(dumpfd);
}


void *Service_2(void *threadp)
{
	struct timeval current_time_val;
	double current_time;
	unsigned long long S2Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	execution_time_img_dump = (double *)malloc(frame_count*sizeof(double));
	start_time_img_dump = (double *)malloc(frame_count*sizeof(double));
	end_time_img_dump = (double *)malloc(frame_count*sizeof(double));


	double start_time;	//To store start time in seconds
	double end_time;	//To store end time in seconds
	double diff_time;	//To store execution time for each iteration

	struct timespec S2_start_time;
	struct timespec S2_end_time;

	while(!abortS2)
	{
		sem_wait(&semS2);

		syslog(LOG_INFO,"IMAGE DUMP START");

		clock_gettime(CLOCK_REALTIME, &S2_start_time);

		start_time = ((double)S2_start_time.tv_sec + (double)((S2_start_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"S2 Count: %lld\t Image Dump start Time: %lf seconds\n",S2Cnt,start_time);

		*(start_time_img_dump + S2Cnt) = start_time;		//Store start time in array

		framecnt++;


		syslog(LOG_INFO,"*********************START PPM DUMP***************************");
	
		dump_ppm(bigbuffer, ((size_of_image*6)/4), framecnt, &frame_time);

		syslog(LOG_INFO,"*********************END PPM DUMP***************************");

		clock_gettime(CLOCK_REALTIME, &S2_end_time);

		end_time = ((double)S2_end_time.tv_sec + (double)((S2_end_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"S2 Count: %lld\t Image Dump End Time: %lf seconds\n",S2Cnt,end_time);

		*(end_time_img_dump + S2Cnt) = end_time;		//Store end time in array

		S2Cnt++;

		syslog(LOG_INFO,"IMAGE DUMP END");

		sem_post(&semS3);
	}

	pthread_exit((void *)0);
}
