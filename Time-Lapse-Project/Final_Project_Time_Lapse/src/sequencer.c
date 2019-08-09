/*@Author       : Om Raheja
 *@File Name    : sequencer.c 
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Sequencer is responsible for scheduling all the real time
 *		  threads to capture an image, dump it into ppm format and
 *		  send it via ethernet, respectively.
 * */


/* User defined header files */
#include "../inc/sequencer.h"

/* Standard C library headers */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

/* Defines */ 
#define TRUE 		(1)
#define NSEC_PER_SEC 	(1000000000)


/* Structures */
typedef struct
{
	int threadIdx;
	int MajorPeriods;
	int frame_count;
	unsigned long long sequencePeriods;
} threadParams_t;


/* External global variables */
extern int abortTest;
extern int abortS1;
extern int abortS2;
extern int abortS3;
extern int abortS4;
extern sem_t semS1,semS2,semS3,semS4;
extern int frame_count;
extern int freq_img_capture;


/* Sequencer function */
void *Sequencer(void *threadp)
{






	struct timespec sequencer_start_time;		//To store start time of sequencer
	double sequencer_start_time_sec;





	struct timespec sequencer_end_time;		//To store end time of sequencer
	double sequencer_end_time_sec;
	double sequencer_wcet;				//To store (wcet) worst case execution time of sequencer		
	double sequencer_execution_time;		//To store execution time of each loop of sequencer
	double sequencer_avg_execution_time;		//To store average execution time of sequencer

	/* If frame capturing rate is 1 hertz */
	if(freq_img_capture == 1)
	{

		struct timespec set_time_1hz;		//for setting time relative to reference time
		struct timespec reference_time_1hz;	//for storing the reference time (i.e. start of the sequencer)


		struct timeval current_time_val;
		struct timespec delay_time = {1,0};	// delay for 1sec, 1 Hz
		//struct timespec delay_time = {0,100000000};	// delay for 100msec, 10Hz
		//struct timespec delay_time = {0,999999980};
		struct timespec remaining_time;
		double current_time;
		double residual;
		int rc, delay_cnt=0;
		unsigned long long seqCnt=0;
		threadParams_t *threadParams = (threadParams_t *)threadp;
		int j = 1;

		do
		{
			delay_cnt=0; residual=0.0;
			syslog(LOG_INFO,"SEQUENCER START");
			/* Calculate start time for sequencer on each iteration until all frames are captured */
			clock_gettime(CLOCK_REALTIME, &sequencer_start_time);
			sequencer_start_time_sec = ((double)sequencer_start_time.tv_sec + (double)((sequencer_start_time.tv_nsec)/(double)1000000000));
			syslog(LOG_INFO,"SEQUENCER START TIME: %lf seconds\n",sequencer_start_time_sec);

			do
			{
				//rc=nanosleep(&delay_time, &remaining_time);

				/**********************************************************************************************************************************/

				if(j == 1)
				{
					clock_gettime(CLOCK_REALTIME, &reference_time_1hz);
					set_time_1hz = reference_time_1hz;
					syslog(LOG_INFO,"REFERENCE TIME: %lf seconds\n",((double)reference_time_1hz.tv_sec + (double)((reference_time_1hz.tv_nsec)/(double)1000000000)));
					j++;
				}

				/* Wrap around condition */
				//if(set_time_1hz.tv_nsec > 900000000)
				//{
				//        set_time_1hz.tv_sec++;
				//        set_time_1hz.tv_nsec = set_time_1hz.tv_nsec - 900000000;
				//}

				//else
				//{
				//set_time_1hz.tv_nsec += 100000000;
				set_time_1hz.tv_sec +=1;
				//}


				clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &set_time_1hz, NULL);

				/**********************************************************************************************************************************/
				if(rc == EINTR)
				{
					residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec / (double)NSEC_PER_SEC);

					if(residual > 0.0) printf("residual=%lf, sec=%d, nsec=%d\n", residual, (int)remaining_time.tv_sec, (int)remaining_time.tv_nsec);

					delay_cnt++;
				}
				else if(rc < 0)
				{
					perror("Sequencer nanosleep");
					exit(-1);
				}

			} while((residual > 0.0) && (delay_cnt < 100));

			seqCnt++;


			if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);


			// Release each service at a sub-rate of the generic sequencer rate

			// Servcie_1 = RT_MAX-1	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS1);

			// Service_2 = RT_MAX-2	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS2);

			// Service_3 = RT_MAX-3	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS3);

			clock_gettime(CLOCK_REALTIME, &sequencer_end_time);
			//syslog(LOG_INFO,"SEQUENCER END TIME: %lf seconds\n",((double)sequencer_end_time.tv_sec + (double)((sequencer_end_time.tv_nsec)/(double)1000000000)));
			sequencer_end_time_sec = ((double)sequencer_end_time.tv_sec + (double)((sequencer_end_time.tv_nsec)/(double)1000000000));
			syslog(LOG_INFO,"SEQUENCER END TIME: %lf seconds\n",sequencer_end_time_sec);


			syslog(LOG_INFO,"SEQUENCER END");



		} while(!abortTest && (seqCnt < frame_count));//   while(!abortTest && (seqCnt < threadParams->sequencePeriods));

		sem_post(&semS1);
		sem_post(&semS2);
		sem_post(&semS3);
		abortS1=TRUE;
		abortS2=TRUE;
		abortS3=TRUE;

		pthread_exit((void *)0);
	}
	else if(freq_img_capture == 10)
	{
		struct timeval current_time_val;		//To store the current time value
		//struct timespec delay_time = {1,0};	// delay for 1sec, 1 Hz
		struct timespec delay_time = {0,100000000};	// delay for 100msec, 10Hz
		struct timespec remaining_time;		
		struct timespec measure_time;
		struct timespec reference_time;		//To store the reference time at which sequencer starts
		struct timespec set_time;			//To store time for clock_nanosleep()
		double current_time;
		double residual;
		int rc, delay_cnt=0;
		unsigned long long seqCnt=0;
		threadParams_t *threadParams = (threadParams_t *)threadp;

		int i = 1;
		do
		{
			delay_cnt=0; residual=0.0;

			do
			{
				syslog(LOG_INFO,"SEQUENCER");
				if(i == 1)
				{
					clock_gettime(CLOCK_REALTIME, &reference_time);
					set_time = reference_time;
					syslog(LOG_INFO,"REFERENCE TIME: %lf seconds\n",((double)reference_time.tv_sec + (double)((reference_time.tv_nsec)/(double)1000000000)));
					i++;
				}

				/* Wrap around condition */
				if(set_time.tv_nsec > 900000000)
				{
					set_time.tv_sec++;
					set_time.tv_nsec = set_time.tv_nsec - 900000000;
				}

				else
				{
					set_time.tv_nsec += 100000000;
				}


				clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &set_time, NULL);




				if(rc == EINTR)
				{
					residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec / (double)NSEC_PER_SEC);

					if(residual > 0.0) printf("residual=%lf, sec=%d, nsec=%d\n", residual, (int)remaining_time.tv_sec, (int)remaining_time.tv_nsec);

					delay_cnt++;
				}
				else if(rc < 0)
				{
					perror("Sequencer nanosleep");
					exit(-1);
				}

			} while((residual > 0.0) && (delay_cnt < 100));

			seqCnt++;


			if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);


			// Release each service at a sub-rate of the generic sequencer rate

			// Servcie_1 = RT_MAX-1	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS1);

			// Service_2 = RT_MAX-2	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS2);

			// Service_3 = RT_MAX-3	@ 1 Hz
			if((seqCnt % 1) == 0) sem_post(&semS3);


		} while(!abortTest && (seqCnt < frame_count));			//while(!abortTest && (seqCnt < threadParams->sequencePeriods));

		sem_post(&semS1);		//Sem post service 1 (image capture)
		sem_post(&semS2);		//Sem post service 2 (image dump)
		sem_post(&semS3);		//Sem post service 3 (send image via socket)
		abortS1=TRUE;		//abort service 1
		abortS2=TRUE;		//abort service 2
		abortS3=TRUE;		//abort service 3

		pthread_exit((void *)0);	//exit sequencer
	}
}

