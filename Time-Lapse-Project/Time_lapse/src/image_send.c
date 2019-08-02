/*@Author       : Om Raheja
 *@File Name    : image_send.c
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Send the image from server to client via sockets. 
 */

/* User defined header files */
#include "../inc/image_send.h"

/* Standard C library headers */
#include <pthread.h>
#include <syslog.h>
#include <semaphore.h>

/* Structures */
typedef struct
{
    int threadIdx;
    int MajorPeriods;
    int frame_count;
    unsigned long long sequencePeriods;
} threadParams_t;


/* External global variables */
extern sem_t semS3;
extern int abortS3;

/* Service 3 [Image send via sockets] */
void *Service_3(void *threadp)
{
    struct timeval current_time_val;
    double current_time;
    unsigned long long S3Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    struct timespec S3_start_time;
    struct timespec S3_end_time;

    while(!abortS3)
    {
        sem_wait(&semS3);
	syslog(LOG_INFO,"SERVICE 3....");

        clock_gettime(CLOCK_REALTIME, &S3_start_time);
        syslog(LOG_INFO,"S3 Count: %lld\t S3 Start Time: %lf seconds\n",S3Cnt,((double)S3_start_time.tv_sec + (double)((S3_start_time.tv_nsec)/(double)1000000000)));
	S3Cnt++;
	clock_gettime(CLOCK_REALTIME, &S3_end_time);
        syslog(LOG_INFO,"S3 Count: %lld\t S3 End Time: %lf seconds\n",S3Cnt,((double)S3_end_time.tv_sec + (double)((S3_end_time.tv_nsec)/(double)1000000000)));

    }

    pthread_exit((void *)0);
}
