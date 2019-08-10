#include "../inc/image_send.h"
#include "../inc/client.h"
#include <pthread.h>
#include <syslog.h>
#include <stdbool.h>
#include <semaphore.h>

typedef struct
{
    int threadIdx;
    int MajorPeriods;
    int frame_count;
    unsigned long long sequencePeriods;
} threadParams_t;



extern sem_t semS3;
extern int abortS3;
extern char ppm_dumpname[];
extern int frame_count;

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
	syslog(LOG_INFO,"SOCKET SEND START");
        clock_gettime(CLOCK_REALTIME, &S3_start_time);
        syslog(LOG_INFO,"S3 Count: %lld\t S3 Start Time: %lf seconds\n",S3Cnt,((double)S3_start_time.tv_sec + (double)((S3_start_time.tv_nsec)/(double)1000000000)));
	
	client_send(ppm_dumpname);
	
	clock_gettime(CLOCK_REALTIME, &S3_end_time);
        syslog(LOG_INFO,"S3 Count: %lld\t S3 End Time: %lf seconds\n",S3Cnt,((double)S3_end_time.tv_sec + (double)((S3_end_time.tv_nsec)/(double)1000000000)));
	S3Cnt++;
	if(S3Cnt == frame_count)
	{
		abortS3 = true;
	}
	syslog(LOG_INFO,"SOCKET SEND END");

    }

    pthread_exit((void *)0);
}
