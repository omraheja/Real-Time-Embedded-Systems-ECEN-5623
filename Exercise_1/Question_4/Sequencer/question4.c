/*@Author	: Dr.Sam Siewert
 *@Reference	: This code has been taken as a reference for the assignment for the course
 		  Real-Time-Embedded-Systems (ECEN 5623). The code has been written completely
		  by Dr.Sam Siewert.The code can be found on Dr.Sam Siewert's course website page.
 *@Date		: 14th June 2019
 *@Board used	: NVIDIA's Jetson Nano running Linux.
 *@Tools	: Compiler:gcc ; Editor: Vim
 * */

/* NOTE: Place #define _GNU_SOURCE before all the #include (s) and specially before #include <sched.h> in order
 * to avoid any error related to undefined reference to CPU_ZERO, CPU_SET, CPU_COUNT macros.
 * Reference: https://stackoverflow.com/questions/24034631/error-message-undefined-reference-for-cpu-zero
 */


/*Check to ensure all your CPU cores on in an online state.

Check /sys/devices/system/cpu or do lscpu.

Tegra is normally configured to hot-plug CPU cores, so to make all available,
as root do:

echo 0 > /sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/enable
echo 1 > /sys/devices/system/cpu/cpu1/online
echo 1 > /sys/devices/system/cpu/cpu2/online
echo 1 > /sys/devices/system/cpu/cpu3/online

Check for precision time resolution and support with cat /proc/timer_list

Ideally all printf calls should be eliminated as they can interfere with
timing.  They should be replaced with an in-memory event logger or at least
calls to syslog.

This is necessary for CPU affinity macros in Linux
*/


/* Standard C Library Headers */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#define USEC_PER_MSEC (1000)
#define NUM_CPU_CORES (1)
#define FIB_TEST_CYCLES (100)
#define NUM_THREADS (3)		/* service threads (Fib10 and Fib20) + sequencer */
sem_t semF10, semF20;

/* Define limit for FIB_TEST macro iteration count */
#define FIB_LIMIT_FOR_32_BIT (47)
#define FIB_LIMIT (10)

/* Global variables */
int abortTest = 0;
double start_time;
unsigned int seqIterations = FIB_LIMIT;
unsigned int idx = 0, jdx = 1;
unsigned int fib = 0, fib0 = 0, fib1 = 1;

/* Function Prototypes */
double getTimeMsec(void);


/* Macros */
#define FIB_TEST(seqCnt, iterCnt)      \
   for(idx=0; idx < iterCnt; idx++)    \
   {                                   \
      fib0=0; fib1=1; jdx=1;           \
      fib = fib0 + fib1;               \
      while(jdx < seqCnt)              \
      {                                \
         fib0 = fib1;                  \
         fib1 = fib;                   \
         fib = fib0 + fib1;            \
         jdx++;                        \
      }                                \
   }                                   \


typedef struct
{
    int threadIdx;
    int MajorPeriods;
} threadParams_t;


/* Iterations, 2nd arg must be tuned for any given target type
   using timestamps
   
   Be careful of WCET overloading CPU during first period of LCM.
   
 */
void *fib10(void *threadp)
{
   double event_time, run_time=0.0;
   int limit=0, release=0, cpucore, i;
   threadParams_t *threadParams = (threadParams_t *)threadp;
   unsigned int required_test_cycles;

   /* Assume FIB_TEST short enough that preemption risk is minimal */
   FIB_TEST(seqIterations, FIB_TEST_CYCLES); /* warm cache */
   event_time=getTimeMsec();
   FIB_TEST(seqIterations, FIB_TEST_CYCLES);
   run_time=getTimeMsec() - event_time;

   required_test_cycles = (int)(10.0/run_time);
   printf("F10 runtime calibration %lf msec per %d test cycles, so %u required\n", run_time, FIB_TEST_CYCLES, required_test_cycles);

   while(!abortTest)
   {
       sem_wait(&semF10);	/* Wait for semaphore until the sequencer thread posts it */

       if(abortTest)
           break; 
       else 
           release++;

       cpucore=sched_getcpu();	/*Get the cpu core being used */
       printf("F10 start %d @ %lf on core %d\n", release, (event_time=getTimeMsec() - start_time), cpucore);

       do
       {
           FIB_TEST(seqIterations, FIB_TEST_CYCLES);
           limit++;
       }
       while(limit < required_test_cycles);

       printf("F10 complete %d @ %lf, %d loops\n", release, (event_time=getTimeMsec() - start_time), limit);
       limit=0;
   }

   pthread_exit((void *)0);
}

/* FIB20 [start routine] */
void *fib20(void *threadp)
{
   /* local variables */
   double event_time, run_time=0.0;
   int limit=0, release=0, cpucore, i;
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int required_test_cycles;

   /* Assume FIB_TEST short enough that preemption risk is minimal */
   /* Calculate the time taken to measure multiple iterations and 
    * test cycles of the fibonacci series.
    * Based on the time taken to measure this (run_time), adjust or calculate
    * the number of test cycles (required_test)cycles) to achieve the required
    * timing.(10ms for Fib10 and 20ms for Fib20).
    * */
   FIB_TEST(seqIterations, FIB_TEST_CYCLES); //warm cache
   event_time=getTimeMsec();
   FIB_TEST(seqIterations, FIB_TEST_CYCLES);
   run_time=getTimeMsec() - event_time;

   required_test_cycles = (int)(20.0/run_time);		/*Required test cycles to generate a specific delay based on run_time*/
   printf("F20 runtime calibration %lf msec per %d test cycles, so %d required\n", run_time, FIB_TEST_CYCLES, required_test_cycles);

   while(!abortTest)
   {
        sem_wait(&semF20);

        if(abortTest)
           break; 
        else 
           release++;

        cpucore=sched_getcpu();
        printf("F20 start %d @ %lf on core %d\n", release, (event_time=getTimeMsec() - start_time), cpucore);

        do
        {
            FIB_TEST(seqIterations, FIB_TEST_CYCLES);
            limit++;
        }
        while(limit < required_test_cycles);

        printf("F20 complete %d @ %lf, %d loops\n", release, (event_time=getTimeMsec() - start_time), limit);
        limit=0;
   }

   pthread_exit((void *)0);
}


/* Returns time in msec
 * Used to mesure time taken for FIB_TEST to run depending on the 
 * arguments (seqIterations and FIB_TEST_CYCLES) */
double getTimeMsec(void)
{
  struct timespec event_ts = {0, 0};

  clock_gettime(CLOCK_MONOTONIC, &event_ts);
  return ((event_ts.tv_sec)*1000.0) + ((event_ts.tv_nsec)/1000000.0);
}


/* Prints the scheduling policy */
void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }

}


/* Scheduler code */
void *Sequencer(void *threadp)
{
  int i;
  int MajorPeriodCnt=0;
  double event_time;
  threadParams_t *threadParams = (threadParams_t *)threadp;

  printf("Starting Sequencer: [S1, T1=20, C1=10], [S2, T2=50, C2=20], U=0.9, LCM=100\n");
  start_time=getTimeMsec();


  /* Sequencing loop for LCM phasing of S1, S2 */
  do
  {

      /* Basic sequence of releases after CI for 90% load
         S1: T1= 20, C1=10 msec 
         S2: T2= 50, C2=20 msec
      
       	This is equivalent to a Cyclic Executive Loop where the major cycle is
       	100 milliseconds with a minor cycle of 20 milliseconds, but here we use
       	pre-emption rather than a fixed schedule.

	As the MajorPeriod is set as 3, this part of the code runs for 3 times.

	Thus, we can see the scheduling of tasks over 3 different ranges,
	0-100ms, 100-200ms and  200-300ms
      */

      /* Simulate the C.I. for S1 and S2 and timestamp in log */
      printf("\n**** CI t=%lf\n", event_time=getTimeMsec() - start_time);
      sem_post(&semF10);
      sem_post(&semF20);

      usleep(20*USEC_PER_MSEC);
      sem_post(&semF10);
      printf("t=%lf\n", event_time=getTimeMsec() - start_time);

      usleep(20*USEC_PER_MSEC);
      sem_post(&semF10);
      printf("t=%lf\n", event_time=getTimeMsec() - start_time);

      usleep(10*USEC_PER_MSEC);
      sem_post(&semF20);
      printf("t=%lf\n", event_time=getTimeMsec() - start_time);

      usleep(10*USEC_PER_MSEC);
      sem_post(&semF10);
      printf("t=%lf\n", event_time=getTimeMsec() - start_time);

      usleep(20*USEC_PER_MSEC);
      sem_post(&semF10);
      printf("t=%lf\n", event_time=getTimeMsec() - start_time);

      usleep(20*USEC_PER_MSEC);

      MajorPeriodCnt++;
   } 
   while (MajorPeriodCnt < threadParams->MajorPeriods);
 
   abortTest=1;		/* Breaks the Fib10 and Fib20 routines */
   sem_post(&semF10); sem_post(&semF20);
}


void main(void)
{
    /* Local Variables*/
    int i, rc, scope;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    cpu_set_t allcpuset;

    abortTest=0;

    /*Display number of processors and number of available processors */
    printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

    /*Clears set, so that it contains no CPUs */
    CPU_ZERO(&allcpuset);

    /* Add CPU cpu to the set */
   for(i=0; i < NUM_CPU_CORES; i++)
       CPU_SET(i, &allcpuset);

   printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));


    /* Initialize the sequencer semaphores */
    if (sem_init (&semF10, 0, 0)) { printf ("Failed to initialize semF10 semaphore\n"); exit (-1); }
    if (sem_init (&semF20, 0, 0)) { printf ("Failed to initialize semF20 semaphore\n"); exit (-1); }

    mainpid=getpid();

    /* Find the range of max priority for the SCHED_FIFO scheduling policy */
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    /* Assign max priority to main thread */
    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;

    /* Set the scheduling policy along with priority */
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    print_scheduler();


    /*Get the scope of the thread */
    pthread_attr_getscope(&main_attr, &scope);

    /* For the thread to use resources from the entire system
     * keep the scope of the thread as the PTHREAD_SCOPE_SYSTEM */
    if(scope == PTHREAD_SCOPE_SYSTEM)
      printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
      printf("PTHREAD SCOPE PROCESS\n");
    else
      printf("PTHREAD SCOPE UNKNOWN\n");

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);

    /*Assign cpu affinity to the thread so that the thread
     * is bounded to that CPU core only  */
    for(i=0; i < NUM_THREADS; i++)
    {

      CPU_ZERO(&threadcpu);
      CPU_SET(3, &threadcpu);

      rc=pthread_attr_init(&rt_sched_attr[i]);
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
      rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

      rt_param[i].sched_priority=rt_max_prio-i;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

      threadParams[i].threadIdx=i;
    }
   
    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));

    // Create Service threads which will block awaiting release for:
    // serviceF10
    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      &rt_sched_attr[1],         // use specific attributes
                      //(void *)0,                 // default attributes
                      fib10,                     // thread function entry point
                      (void *)&(threadParams[1]) // parameters to pass in
                     );
    // serviceF20
    rc=pthread_create(&threads[2],               // pointer to thread descriptor
                      &rt_sched_attr[2],         // use specific attributes
                      //(void *)0,                 // default attributes
                      fib20,                     // thread function entry point
                      (void *)&(threadParams[2]) // parameters to pass in
                     );


    // Wait for service threads to calibrate and await relese by sequencer
    usleep(300000);
 
    // Create Sequencer thread, which like a cyclic executive, is highest prio
    printf("Start sequencer\n");
    threadParams[0].MajorPeriods=3;

    rc=pthread_create(&threads[0],               // pointer to thread descriptor
                      &rt_sched_attr[0],         // use specific attributes
                      //(void *)0,                 // default attributes
                      Sequencer,                 // thread function entry point
                      (void *)&(threadParams[0]) // parameters to pass in
                     );


    /* Wait until all threads finish their execution */
   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);


   printf("\nTEST COMPLETE\n");

}
