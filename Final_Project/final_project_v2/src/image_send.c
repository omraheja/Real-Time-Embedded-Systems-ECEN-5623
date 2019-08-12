/*@Author       : Om Raheja
 *@File Name    : image_send.c
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief        : Converts the obtained data in bugbuffer into ppm files.
 */

/* User defined header files */
#include "../inc/image_send.h"
#include "../inc/client.h"

/* Standardy C library headers */
#include <pthread.h>
#include <syslog.h>
#include <stdbool.h>
#include <semaphore.h>

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
extern int abortS3;
extern char ppm_dumpname[];
extern int frame_count;
extern int socket_enable;


/* Image send service */
void *Service_3(void *threadp)
{
    struct timeval current_time_val;
    double current_time;
    unsigned long long S3Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    struct timespec S3_start_time;		//To measure start time of the service
    struct timespec S3_end_time;		//To measure end time of the service

    while(!abortS3)
    {
        sem_wait(&semS3);			//Wait on the semaphore until released by the sequencer
	syslog(LOG_INFO,"SOCKET SEND START");
        
	clock_gettime(CLOCK_REALTIME, &S3_start_time);	//Get start time of the service
        syslog(LOG_INFO,"S3 Count: %lld\t S3 Start Time: %lf seconds\n",S3Cnt,((double)S3_start_time.tv_sec + (double)((S3_start_time.tv_nsec)/(double)1000000000)));
	
	if(socket_enable == 1)
	{
		client_send(ppm_dumpname);
	}
	
	clock_gettime(CLOCK_REALTIME, &S3_end_time);	//Get end time of the service
	syslog(LOG_INFO,"S3 Count: %lld\t S3 End Time: %lf seconds\n",S3Cnt,((double)S3_end_time.tv_sec + (double)((S3_end_time.tv_nsec)/(double)1000000000)));
	
	S3Cnt++;					//Increment the count of service S3

	if(S3Cnt == frame_count)
	{
		abortS3 = true;
	}
	syslog(LOG_INFO,"SOCKET SEND END");

    }

    pthread_exit((void *)0);
}
