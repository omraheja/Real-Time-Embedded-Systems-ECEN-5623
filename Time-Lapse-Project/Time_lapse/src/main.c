/*@Author	: Om Raheja
 *@Reference	: The code for image capturing and frame acquisition has been referenced from
 *                the official website of V4L2(Video for Linux). The v4l2 driver code can also
 *                be found on Dr.Sam Siewert's course website page.
 *                
 *                Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *                grabber NTSC cameras to acquire digital video from a source,
 *                time-stamp each frame acquired, save to a PGM or PPM file.
 *                
 *                The original code adapted was open source from V4L2 API and had the
 *                following use and incorporation policy:
 *                This program can be used and distributed without restrictions.
 *                This program is provided with the V4L2 API see http://linuxtv.org/docs.php for more information
 *
 *                Sequencer function was referenced from Professor Sam Siewert's course page. 
 *
 *		  The code was socket was referenced from stack overflow, link for which is provided below
 *		  https://stackoverflow.com/questions/15445207/sending-image-jpeg-through-socket-in-c-linux
 *		  The code has been explained in <filename>
 *
 *@File Name	: main.c 
 *@Date		: 
 *@Board used	: NVIDIA's Jetson Nano running Linux.
 *@Tools	: Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 * */


/* NOTE: Place #define _GNU_SOURCE before all the #include (s) and specially before #include <sched.h> in order
 * to avoid any error related to undefined reference to CPU_ZERO, CPU_SET, CPU_COUNT macros.
 * #define _GNU_SOURCE is located in the 'main.h' file in the 'inc' sub-folder.
 * Reference: https://stackoverflow.com/questions/24034631/error-message-undefined-reference-for-cpu-zero
 */

/* User defined header files */
#include "../inc/main.h"
#include "../inc/v4l2_driver.h"
#include "../inc/sequencer.h"
#include "../inc/image_capture.h"
#include "../inc/ppm_dump.h"
#include "../inc/image_send.h"
#include "../inc/print_scheduler.h"

/* Defines */
#define NUM_THREADS	(4)
#define NUM_CPU_CORES	(1)
#define OK 		(1)
#define K 		(4.0)
#define USEC_PER_MSEC 	(1000)
#define NSEC_PER_SEC 	(1000000000)
#define ERROR 		(-1)
#define TRUE		(1)
#define FALSE		(0)

/* Structures */
typedef struct
{
    int threadIdx;
    int MajorPeriods;
    int frame_count;
    unsigned long long sequencePeriods;
} threadParams_t;


struct buffer
{
        void   *start;
        size_t  length;
};


/* Global variables */
char *dev_name;		//To store device name				[command line argument]
char hres_str[3];	//To store horizontal resolution as a string	[command line argument]
char vres_str[3];	//To store vertical resolution as a string	[command line argument]
int  frame_count;	//To store the number of frames to be captured  [command line argument]
int  HRES;		//To store Horizontal resolution		[command line argument]
int  VRES;		//To store Vertical resolution			[command line argument]
int  size_of_image;	//To store size of image for use in different functions

/* PPM header */
char ppm_header[50];
char ppm_dumpname[]="Test00000000.pgm";


int abortTest	= FALSE;
int abortS1	= FALSE;
int abortS2	= FALSE;
int abortS3	= FALSE;
int abortS4	= FALSE;

/* Semaphores */
sem_t semS1,semS2,semS3,semS4;

/* Variables required for v4l2 driver */
int force_format = 1;
int out_buf;
int fd = -1;
unsigned int n_buffers;

/* Structure buffer instance */
struct buffer *buffers;

/* To keep a count of the number of frames captured */
unsigned int  framecnt=0;

/* Array for storing the image */
unsigned char bigbuffer[(1280*960*3)];

/* v4l2_buffer and v4l2_format instances for v4l2 driver functions */
struct v4l2_buffer buf;
struct v4l2_format fmt;		//Format is used by a number of functions, so made as a file global


/* Main Function */
int main(int argc, char **argv)
{
	int rc,scope,rt_max_prio,rt_min_prio;
	cpu_set_t threadcpu;
	pthread_t threads[NUM_THREADS];
	threadParams_t threadParams[NUM_THREADS];
        pthread_attr_t rt_sched_attr[NUM_THREADS];
        struct sched_param rt_param[NUM_THREADS];
        struct sched_param main_param;
        pthread_attr_t main_attr;
        pid_t mainpid;
        cpu_set_t allcpuset;
	
	printf("**********************************STARTING TIME LAPSE PROGRAM****************************************\n");

	if(argc > 1)
	{
		dev_name = argv[1];		//Store device path in dev_name
		frame_count = atoi(argv[2]);	//Store frame count entered as command line argument
		HRES = atoi(argv[3]);		//Store Horizontal resolution
		VRES = atoi(argv[4]);		//Store Vertical resolution

		/* Check if proper resolutions are entered in the command line arguments */
		if(HRES != 640 || VRES != 480)
		{
			printf("[ERROR]:Supported Resolutions: 640x480\n");
			exit(-1);
		}

		strncpy(hres_str,argv[3],sizeof(hres_str));
		strncpy(vres_str,argv[4],sizeof(vres_str));
		printf("FRAME COUNT = %d\n",frame_count);
		printf("RESOLUTION = %dx%d\n",HRES,VRES);
		sprintf(ppm_header,"P6\n#9999999999 sec 9999999999 msec \n%s %s\n255\n",hres_str,vres_str);
	}
	else
	{
		printf("USAGE: [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES]\n");
		exit(-1);
	}

	/*Display number of processors and number of available processors */
        printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

	/* Clears set, so that it contains no CPU */
	CPU_ZERO(&allcpuset);

	/* Add CPU 'cpu' to the set */
	for(int i=0;i< NUM_CPU_CORES;i++)
	{
		CPU_SET(i,&allcpuset);
	}

	printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));

	/* Find the range of max priority for the SCHED_FIFO scheduling policy */
        rt_max_prio = sched_get_priority_max(SCHED_FIFO);
        rt_min_prio = sched_get_priority_min(SCHED_FIFO);

	printf("rt_max_prio=%d\n", rt_max_prio);
        printf("rt_min_prio=%d\n", rt_min_prio);

	mainpid=getpid(); 

	/* Assign max priority to main thread */
        rc=sched_getparam(mainpid, &main_param);
	if(rc < 0)
	{
		perror("[MAIN]:Error in shced_getparam()");
	}

	/* Assign maximum priority to main thread */
        main_param.sched_priority = rt_max_prio;

	/* Set the scheduling policy along with priority */
        rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
        if(rc < 0)
	{
		perror("[MAIN]:Error in sched_setscheduler()");
	}

        print_scheduler();		/* Print the scheduling policy */


	/*Get the scope of the thread */
        pthread_attr_getscope(&main_attr, &scope);

        /* For the thread to use resources from the entire system
         * keep the scope of the thread as the PTHREAD_SCOPE_SYSTEM
	 * else can keep PTHREAD_SCOPE_PROCESS.
	 */
        if(scope == PTHREAD_SCOPE_SYSTEM)
	{
		printf("PTHREAD SCOPE SYSTEM\n");
	}
        else if (scope == PTHREAD_SCOPE_PROCESS)
	{
		printf("PTHREAD SCOPE PROCESS\n");
	}
        else
	{
		printf("PTHREAD SCOPE UNKNOWN\n");
	}

	/* Assign cpu affinity to the thread so that the thread
         * is bounded to that CPU core only
	 * */
        for(int i=0; i < NUM_THREADS; i++)
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


    	/* initialize the sequencer semaphores */
    	if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    	if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    	if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
    	if (sem_init (&semS4, 0, 0)) { printf ("Failed to initialize S4 semaphore\n"); exit (-1); }

	open_device();		/* Open device */
        init_device();		/* Initialize the device */
        start_capturing();	/* Start capturing the images  */

	
	/* Create Service threads which will block awaiting release for: */
	/* Service_1 = RT_MAX-1	@ 1 Hz
	 * Service_1 = Captures images at a rate of either 1 hertz or 10 hertz. */
	rt_param[1].sched_priority=rt_max_prio-1;
	pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
	rc=pthread_create(&threads[1], &rt_sched_attr[1], Service_1, (void *)&(threadParams[1]));
	if(rc < 0)
		perror("pthread_create for service 2");
	else
		printf("pthread_create successful for service 2\n");

	/* Service_2 = RT_MAX-2 @ 1 Hz
	 * Service_2 = Converts collected data into images of .ppm format */
        rt_param[2].sched_priority=rt_max_prio-2;
        pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
        rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2, (void *)&(threadParams[2]));
        if(rc < 0)
                perror("pthread_create for service 2");
        else
                printf("pthread_create successful for service 2\n");

	/* Service_3 = RT_MAX-3 @ 1 Hz
	 * Service_3 = Sends images via socket to client */
        rt_param[3].sched_priority=rt_max_prio-3;
        pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
        rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3, (void *)&(threadParams[3]));
        if(rc < 0)
                perror("pthread_create for service 2");
        else
                printf("pthread_create successful for service 2\n");


	/* Note that the sleep is not necessary of RT service threads are created wtih
	 * correct POSIX SCHED_FIFO priorities compared to non-RT priority of this main
	 * program.
	 */
	//usleep(1000000);

	/* Create Sequencer thread, which like a cyclic executive, is highest priority */
	threadParams[0].sequencePeriods=900;

	/* Sequencer = RT_MAX	@ 1 Hz 
	 * Sequencer = This is the highest priority thread that schedules the other 
	 * real time threads in order to maintain its timing.*/
    	rt_param[0].sched_priority = rt_max_prio;
    	pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    	rc=pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, (void *)&(threadParams[0]));
    	if(rc < 0)
    	    perror("pthread_create for sequencer service 0");
    	else
    	    printf("pthread_create successful for sequeencer service 0\n");


	/* Wait for threads to join */
	for(int i=0;i<NUM_THREADS;i++)
	{
		pthread_join(threads[i], NULL);
	}

	stop_capturing();	/* Stop Capturing images */
    	uninit_device();	/* Free resources allocated for use of this application */
    	close_device();		/* Close the device */
	
	printf("**********************************ENDING TIME LAPSE PROGRAM****************************************\n");
	return 0;
}
