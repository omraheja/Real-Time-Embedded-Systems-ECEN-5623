/*@Author       : Om Raheja
 *@Date         : 4th July 2019 
 *@Filename     : thread.c
 *@Course       : Real Time Embedded Systems
 *@Board Used   : NVIDIA's Jetson Nano
 *@Tools used   : Gcc compiler; Vim Editor; MobaXTerm
 *@Reference	: This code has beem modified for assignment purposes.
 		  The sample code for this was written by Dr.Sam Siewert.
		  The example code can be found at the below website:
		  http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/example-sync/
 * */


/* Standard C Library Headers */
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Macros */
#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2


/* Global Variables */
pthread_t threads[NUM_THREADS];
struct sched_param nrt_param;
pthread_mutex_t rsrcA, rsrcB;
volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;
struct timespec wait_for_B;
struct timespec wait_for_A;


/*@Function Name : grabRsrcs
 *@Param in	 : threadid 
 *@Return	 : void
 * */
void *grabRsrcs(void *threadid)
{
   if((int)threadid == THREAD_1)
   {
	   printf("THREAD 1 grabbing resources\n");
	   pthread_mutex_lock(&rsrcA);			//Lock resource A
	   rsrcACnt++;
	   if(!noWait) usleep(1000000);
	   printf("THREAD 1 got A, trying for B\n");
	   
	   clock_gettime(CLOCK_REALTIME,&(wait_for_B));
	   wait_for_B.tv_sec += (rand()) % 10;
	   
	   int rcB = pthread_mutex_timedlock(&rsrcB,&wait_for_B);	//Wait on timedMutex for Resource B
	   if(rcB != 0)
	   {
		   pthread_mutex_unlock(&rsrcA);	//Unlock resource A
		   sleep(2);					
		   pthread_mutex_lock(&rsrcA);		//Lock resource A
		   pthread_mutex_lock(&rsrcB);		//Lock resource B
	   }
	   else
	   {
		   rsrcBCnt++;
		   printf("THREAD 1 got A and B\n");
		   pthread_mutex_unlock(&rsrcB);	//Unlock resource B before exiting function
		   pthread_mutex_unlock(&rsrcA);	//Unlock resource A before exiting function
		   printf("THREAD 1 done\n");
	   }
   }
   else
   {
	   printf("THREAD 2 grabbing resources\n");
	   pthread_mutex_lock(&rsrcB);			//Lock resource B
	   rsrcBCnt++;
	   if(!noWait) usleep(1000000);
	   printf("THREAD 2 got B, trying for A\n");
	   
	   clock_gettime(CLOCK_REALTIME,&(wait_for_A));
	   wait_for_A.tv_sec += (rand()) % 10;
	   int rcA = pthread_mutex_timedlock(&rsrcA,&wait_for_A);	//Wait on timedMutex for Resource A
	   if(rcA != 0)
	   {
		   pthread_mutex_unlock(&rsrcB);	//Unlock resource B
		   sleep(2);
		   pthread_mutex_lock(&rsrcB);		//Lock resource B
		   pthread_mutex_lock(&rsrcA);		//Lock resource A
	   }
	   else
	   {
		   rsrcACnt++;
		   printf("THREAD 2 got B and A\n");
		   pthread_mutex_unlock(&rsrcA);	//Unlock resource A before exiting function
		   pthread_mutex_unlock(&rsrcB);	//Unlock resource B before exiting function
		   printf("THREAD 2 done\n");
	   }
   }
}


int main (int argc, char *argv[])
{
   int rc, safe=0;

   rsrcACnt=0, rsrcBCnt=0, noWait=0;

   srand(time(NULL));

   if(argc < 2)
   {
     printf("Will set up unsafe deadlock scenario\n");
   }
   else if(argc == 2)
   {
     if(strncmp("safe", argv[1], 4) == 0)
       safe=1;
     else if(strncmp("race", argv[1], 4) == 0)
       noWait=1;
     else
       printf("Will set up unsafe deadlock scenario\n");
   }
   else
   {
     printf("Usage: deadlock [safe|race|unsafe]\n");
   }

   // Set default protocol for mutex
   pthread_mutex_init(&rsrcA, NULL);
   pthread_mutex_init(&rsrcB, NULL);

   printf("Creating thread %d\n", THREAD_1);
   rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)THREAD_1);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 1 spawned\n");

   if(safe) // Make sure Thread 1 finishes with both resources first
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %ld done\n", threads[0]);
     else
       perror("Thread 1");
   }

   printf("Creating thread %d\n", THREAD_2);
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)THREAD_2);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 2 spawned\n");

   printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
   printf("will try to join CS threads unless they deadlock\n");

   if(!safe)
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %ld done\n", threads[0]);
     else
       perror("Thread 1");
   }

   if(pthread_join(threads[1], NULL) == 0)
     printf("Thread 2: %ld done\n", threads[1]);
   else
     perror("Thread 2");

   if(pthread_mutex_destroy(&rsrcA) != 0)
     perror("mutex A destroy");

   if(pthread_mutex_destroy(&rsrcB) != 0)
     perror("mutex B destroy");

   printf("All done\n");

   exit(0);
}
