/*@Author       : Om Raheja
 *@File Name    : image_store.c 
 *@Date         : 8/11/2019 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief        : Reads the frames from the buffer and stores it into a circular buffer.
 */

/* User defined header files */
#include "../inc/image_store.h"

/* Standard C library headers */
#include <stdio.h>
#include <stdlib.h>

/* Defines */
#define EXTRA_FRAMES	(20)


/* External global variables */
extern int frame_count;
extern int abortS2;                     //Made true when service 4 task is complete
extern sem_t semS2;                     //Semaphore for synchronization
extern int abortS3;			//Made true when service 3 is complete
extern sem_t semS3;
extern unsigned char bigbuffer[(640*480*3)];    //Buffer to store data captured by camera

/* Global variables */
unsigned char image_store[60][640*480*3];
int head = 0;

/* Structures */
typedef struct
{
	int threadIdx;
	int MajorPeriods;
	int frame_count;
	unsigned long long sequencePeriods;
} threadParams_t;


double *execution_time_image_store;	//To store execution time for each iteration
double *start_time_image_store;		//To store start time for each iteration
double *end_time_image_store;		//To store end time for each iteration

/* Image store service */
void *Service_2(void *threadp)
{
	unsigned long long S2Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	struct timespec S2_start_time;              //To measure start time of the service
	struct timespec S2_end_time;                //To measure end time of the service

	double start_time;	//To store start time in seconds
	double end_time;	//To store end time in seconds
	double diff_time;	//To store execution time for each iteration

	execution_time_image_store = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));
	start_time_image_store = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));
	end_time_image_store = (double *)malloc((frame_count + EXTRA_FRAMES)*sizeof(double));

	while(S2Cnt < (frame_count + EXTRA_FRAMES ))
	{
		sem_wait(&semS2);                       //Wait on the semaphore until released by the sequencer
		syslog(LOG_INFO,"Image store start");

		clock_gettime(CLOCK_REALTIME, &S2_start_time);  //Get start time of the service

		start_time = ((double)S2_start_time.tv_sec + (double)((S2_start_time.tv_nsec)/(double)1000000000));	//Store start time in seconds

		syslog(LOG_INFO,"S2 Count: %lld\t S2 Start Time: %lf seconds\n",S2Cnt,start_time);

		*(start_time_image_store + S2Cnt) = start_time;		//Store start time in array

		/* Copy image data byte by byte into the circular buffer */
		for(int i=0;i<(640*480*3);i++)
		{
			image_store[S2Cnt % 60][i] = bigbuffer[i];
		}

		head++;		//increment head to keep track of number of images dumpped in circular buffer

		syslog(LOG_INFO,"Head = %d\n",head);

		clock_gettime(CLOCK_REALTIME, &S2_end_time);    //Get end time of the service

		end_time = ((double)S2_end_time.tv_sec + (double)((S2_end_time.tv_nsec)/(double)1000000000));		//Store end time in seconds

		syslog(LOG_INFO,"S2 Count: %lld\t Image Store End Time: %lf seconds\n",S2Cnt,end_time);

		*(end_time_image_store + S2Cnt) = end_time;		//Store end time in array

		S2Cnt++;                                        //Increment the count of service S2

		syslog(LOG_INFO,"Image store end");

		sem_post(&semS3);

	}

	printf("S2 THREAD EXITED\n");

	syslog(LOG_INFO,"[IMAGE STORE]: ********************<THREAD EXITED>********************");

	pthread_exit((void *)0);
}

