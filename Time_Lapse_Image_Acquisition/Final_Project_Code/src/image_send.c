/*@Author       : Om Raheja
 *@File Name    : image_send.c
 *@Date         : 8/13/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief        : Converts the obtained data in bigbuffer into ppm files.
 */

/* User defined header files */
#include "../inc/image_send.h"
#include "../inc/client.h"

/* Standardy C library headers */
#include <pthread.h>
#include <syslog.h>
#include <stdbool.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

/* Defines */
#define EXTRA_FRAMES	(20)


/* Structures */
typedef struct
{
	int threadIdx;
	int MajorPeriods;
	int frame_count;
	unsigned long long sequencePeriods;
} threadParams_t;


/* External Global Variables */
extern sem_t semS3;
extern sem_t semS4;
extern int frame_count;
extern int socket_enable;


/* String to be manipulated to save all the images being received with unique name */
char filename[] = "Test00000000.ppm";

/* Image send service */
void *Service_4(void *threadp)
{
	int num = 1;				//Used for filename
	unsigned long long S4Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	struct timespec S4_start_time;		//To measure start time of the service
	struct timespec S4_end_time;		//To measure end time of the service

	double socket_send_start;		//To store start time of service in seconds
	double socket_send_end;			//To store end time of service in seconds

	/* Check if socket feature is enabled or not.
	 * If (not enabled), then, exit the thread
	 * */
	if(socket_enable == 0)
	{
		S4Cnt = (frame_count + EXTRA_FRAMES) + 1;
	}

	/* Execute this loop until all frames are transfered via the ethernet */
	while(S4Cnt < (frame_count + EXTRA_FRAMES))
	{
		sem_wait(&semS4);			//Wait on the semaphore until released by the sequencer

		clock_gettime(CLOCK_REALTIME, &S4_start_time);  //Get start time of the service
		socket_send_start = ((double)S4_start_time.tv_sec + (double)((S4_start_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"[SOCKET SEND]: [START] [TIMESTAMP : %lf seconds] [Image Sent : %d]",socket_send_start,num);

		snprintf(&filename[4], 9, "%08d", num);
		strncat(&filename[12], ".ppm", 5);


		/* Check if socket feature is enabled or not */
		if(socket_enable == 1)
		{
			client_send(filename);
		}


		S4Cnt++;					//Increment the count of service S3

		clock_gettime(CLOCK_REALTIME, &S4_end_time);  //Get start time of the service
		socket_send_end = ((double)S4_end_time.tv_sec + (double)((S4_end_time.tv_nsec)/(double)1000000000));

		syslog(LOG_INFO,"[SOCKET SEND]: [END] [TIMESTAMP : %lf seconds] [Image Sent : %d]",socket_send_end,num);
		
		num++;
	}

	printf("S4 THREAD EXITED\n");

	syslog(LOG_INFO,"[SOCKET SEND]: ********************<THREAD EXITED>********************");

	pthread_exit((void *)0);
}
