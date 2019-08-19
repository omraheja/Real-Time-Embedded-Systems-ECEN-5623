/*@Author       : Om Raheja
 *@File Name    : ppm_dump.c
 *@Date         : 8/11/2019
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
#include <sys/utsname.h>


/* Defines */
#define EXTRA_FRAMES	(20)

/* External global variables */
extern struct timespec frame_time;	//For timestamp
extern unsigned int  framecnt;		//Store frame number for ppm header
extern int size_of_image;		//To store size of image for use in different functions
extern unsigned char bigbuffer[(640*480*3)];
extern unsigned char image_store[60][(640*480*3)];
extern sem_t semS2;
extern int abortS2;
extern int abortS3;
extern int abortS4;
extern sem_t semS3;
extern sem_t semS4;

extern int frame_count;
extern int socket_enable;


int tail = 0;

/* PPM header */
struct utsname host_name;
extern char ppm_header[200];
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
	snprintf(ppm_header+36, 65,"%s",(host_name.version));
	snprintf(ppm_header+80, 65,"%s",(host_name.nodename));

	//sestrncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

	//syslog(LOG_INFO,"************************write ppm header*************************");

	written=write(dumpfd, ppm_header, sizeof(ppm_header));
	//syslog(LOG_INFO,"*******************ppm header written*******************");

	total=0;

	do
	{
		written=write(dumpfd, p, size);
		total+=written;
	} while(total < size);

	//syslog(LOG_INFO,"******************after do while***********************");

	//printf("wrote %d bytes\n", total);
	close(dumpfd);
}


void *Service_3(void *threadp)
{
	struct timeval current_time_val;
	double current_time;
	unsigned long long S3Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	execution_time_img_dump = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));
	start_time_img_dump = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));
	end_time_img_dump = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));


	double start_time;	//To store start time in seconds
	double end_time;	//To store end time in seconds
	double diff_time;	//To store execution time for each iteration

	struct timespec S3_start_time;
	struct timespec S3_end_time;


	uname(&host_name);

	//while(!abortS3)
	while(S3Cnt < (frame_count + EXTRA_FRAMES))
	{
		sem_wait(&semS3);

		syslog(LOG_INFO,"IMAGE DUMP START");

		clock_gettime(CLOCK_REALTIME, &S3_start_time);

		start_time = ((double)S3_start_time.tv_sec + (double)((S3_start_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"S3 Count: %lld\t Image Dump start Time: %lf seconds\n",S3Cnt,start_time);

		*(start_time_img_dump + S3Cnt) = start_time;		//Store start time in array

		framecnt++;


		//	syslog(LOG_INFO,"*********************START PPM DUMP***************************");

		//dump_ppm(bigbuffer, ((size_of_image*6)/4), framecnt, &frame_time);


		syslog(LOG_INFO,"DUMP IMAGE................................................................");
		dump_ppm( (image_store + (S3Cnt % 60)), ((size_of_image*6)/4), framecnt, &frame_time);
		syslog(LOG_INFO,"DUMPED IMAGE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

		tail++;

		syslog(LOG_INFO,"Tail = %d\n",tail);

		//	syslog(LOG_INFO,"*********************END PPM DUMP***************************");

		clock_gettime(CLOCK_REALTIME, &S3_end_time);

		end_time = ((double)S3_end_time.tv_sec + (double)((S3_end_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"S3 Count: %lld\t Image Dump End Time: %lf seconds\n",S3Cnt,end_time);

		*(end_time_img_dump + S3Cnt) = end_time;		//Store end time in array

		S3Cnt++;

		syslog(LOG_INFO,"IMAGE DUMP END");

		if(socket_enable == 1)
		{

			sem_post(&semS4);
		}
	}

	printf("S3 THREAD EXITED\n");

	pthread_exit((void *)0);
}
