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
 *@Date		: 8/10/2019
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
#include "../inc/client.h"

/* Defines */
#define NUM_THREADS	(4)
#define NUM_CPU_CORES	(1)
#define OK 		(1)
#define K 		(4.0)
#define USEC_PER_MSEC 	(1000)
#define SEC_TO_MSEC	(1000)
#define NSEC_PER_SEC 	(1000000000)
#define NSEC_PER_USEC	(1000000)
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
int  freq_img_capture;	//To store Frequency at which sequencer will run[command line argument]
int  size_of_image;	//To store size of image for use in different functions


/* PPM header */
char ppm_header[50];				//To store contents of header of ppm image
char ppm_dumpname[]="Test00000000.ppm";		//File name


int abortTest	= FALSE;	//Conditional Flag to abort all tests	
int abortS1	= FALSE;	//Conditional Flag to abort Image capture service
int abortS2	= FALSE;	//Conditional Flag to abort ppm dump service
int abortS3	= FALSE;	//Conditional Flag to abort socket service
int abortS4	= FALSE;

/* Semaphores */
sem_t semS1,semS2,semS3,semS4;	//Semaphores to achieve synchronization

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
unsigned char bigbuffer[(640*480*3)];	//Buffer to store data captured by image capture service

/* v4l2_buffer and v4l2_format instances for v4l2 driver functions */
struct v4l2_buffer buf;
struct v4l2_format fmt;		//Format is used by a number of functions, so made as a file global


/* File related variables for dumping analysis into excel file */
static FILE *fptr_S1;		//File pointer to file consisiting of image capture analysis
static FILE *fptr_S2;		//File pointer to file consisting of image dump analysis
static FILE *fptr_S3;
static FILE *fptr_seq;		//File pointer to file consisting of sequencer analysis
char filename_S1[] = "Image_Capture_Jitter_Analysis.csv";		//Image capture file name
char filename_S2[] = "Image_Dump_Jitter_Analysis.csv";			//Image dump file name
char filename_S3[] = "Image_Send_Jitter_Analysis.csv";			//Image send file name
char filename_seq[]= "Sequencer_Jitter_Analysis.csv";			//Sequencer file name


double wcet_img_capture;			//Store worst case execution for image capturing
double total_exec_time_img_capture;		//Store average execution time
double jitter_img_capture;			//Store jitter for image capture
extern double *execution_time_img_capture;	//To store execution time for each iteration
extern double *start_time_img_capture;		//To store start time for each iteration
extern double *end_time_img_capture;		//To store end time for each iteration

double wcet_img_dump;			//Store worst case execution for image capturing
double total_exec_time_img_dump;	//Store average execution time
double jitter_img_dump;			//Store jitter for image capture
extern double *execution_time_img_dump;	//To store execution time for each iteration
extern double *start_time_img_dump;	//To store start time for each iteration
extern double *end_time_img_dump;	//To store end time for each iteration



double wcet_seq;			//Store worst case execution for image capturing
double total_exec_time_seq;		//Store average execution time
double jitter_seq;			//Store jitter for image capture
extern double *execution_time_seq;	//To store execution time for each iteration
extern double *start_time_seq;		//To store start time for each iteration
extern double *end_time_seq;		//To store end time for each iteration


struct timespec delay_1hz = {1,0};		//For jitter analysis when system runs at 1 hertz
struct timespec delay_10hz = {0,100000000};	//For jitter analysis when system runs at 10 hertz

extern int sock;

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
		dev_name = argv[1];			//Store device path in dev_name
		frame_count = atoi(argv[2]);		//Store frame count entered as command line argument
		HRES = atoi(argv[3]);			//Store Horizontal resolution
		VRES = atoi(argv[4]);			//Store Vertical resolution
		freq_img_capture = atoi(argv[5]);	//Store Frequency at which sequencer will run


		/* Check for valid frame number */
		if(frame_count <= 0)
		{
			printf("[ERROR]:Enter a positive frame number greater than 0\n");
			exit(-1);
		}

		/* Check if proper resolutions are entered in the command line arguments */
		if(HRES != 640 || VRES != 480)
		{
			printf("[ERROR]:Supported Resolutions: 640x480\n");
			exit(-1);
		}

		/* Check if frequency entered by user is supported by the application */
		if(freq_img_capture != 1)
		{
			if(freq_img_capture != 10)
			{
				printf("[ERROR]:Supported Frequencies: 1Hz or 10Hz\n");
				exit(-1);
			}
		}
		else if(freq_img_capture != 10)
		{
			if(freq_img_capture != 1)
			{
				printf("[ERROR]:Supported Frequencies: 1Hz or 10Hz\n");
				exit(-1);
			}
		}

		strncpy(hres_str,argv[3],sizeof(hres_str));
		strncpy(vres_str,argv[4],sizeof(vres_str));
		printf("FRAME COUNT = %d\n",frame_count);
		printf("RESOLUTION = %dx%d\n",HRES,VRES);
		printf("FREQUENCY OF SEQUENCER = %d Hertz\n",freq_img_capture);
		sprintf(ppm_header,"P6\n#9999999999 sec 9999999999 msec \n%s %s\n255\n",hres_str,vres_str);
	}
	else
	{
		printf("USAGE: [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES | Frame Frequency (1hz or 10 hz)]\n");
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

	/* Print the max and min piority for SCHED_FIFO */
	printf("rt_max_prio=%d\n", rt_max_prio);
	printf("rt_min_prio=%d\n", rt_min_prio);

	/* Get process ID of main */
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
	init_client();		/* Initialize Client */


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

	close(sock);
	stop_capturing();	/* Stop Capturing images */
	uninit_device();	/* Free resources allocated for use of this application */
	close_device();		/* Close the device */

	printf("**********************************ENDING TIME LAPSE PROGRAM****************************************\n\n");
	
	
	print_sequencer_analysis();	
	
	print_image_capture_analysis();

	print_image_dump_analysis();



	fclose(fptr_S1);
	fclose(fptr_S2);
	fclose(fptr_seq);






	return 0;
}




void print_sequencer_analysis()
{
	printf("********************************JITTER ANALYSIS SEQUENCER START********************************\n");

	total_exec_time_seq = 0;

	fptr_seq = fopen(filename_seq,"w+");

	fprintf(fptr_seq,"Sequencer Count,Start_Time(in sec),End_Time(in sec),Execution Time(in msec),Jitter(in msec)");

	/* Calculate execution time, WCET and average execution time */
	for(int i=0;i<frame_count;i++)
	{
		/* Calculate execution time of each iteration of sequencer (in secs) */
		*(execution_time_seq + i) = (*(end_time_seq + i) - *(start_time_seq + i))*SEC_TO_MSEC;

		if(i == 10)
		{
			wcet_seq = *(execution_time_seq + i);
		}

		if(wcet_seq < *(execution_time_seq + i))
		{
			wcet_seq = *(execution_time_seq + i);
		}

		/* Calculate total time of execution for image capture thread */
		total_exec_time_seq += *(execution_time_seq + i);

	}

	for(int i=0;i<frame_count;i++)
	{

		if(freq_img_capture == 1)
		{
			if(i == 0)
			{
				jitter_seq = 0;
				fprintf(fptr_seq,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_seq + i),*(end_time_seq + i),*(execution_time_seq + i),jitter_seq*SEC_TO_MSEC);
			}
			else
			{
				jitter_seq = (*(start_time_seq + i - 1) + delay_1hz.tv_sec) - (*(start_time_seq + i)) ;
				fprintf(fptr_seq,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_seq + i),*(end_time_seq + i),*(execution_time_seq + i),jitter_seq*SEC_TO_MSEC);      //Write all values calculated in file


			}
		}
		else if (freq_img_capture == 10)
		{
			if(i == 0)
			{
				jitter_seq = 0;
				fprintf(fptr_seq,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_seq + i),*(end_time_seq + i),*(execution_time_seq + i),jitter_seq);
			}
			else
			{
				jitter_seq = (*(start_time_seq + i - 1) + (delay_10hz.tv_nsec/NSEC_PER_SEC)) - (*(start_time_seq + i)) ;
				fprintf(fptr_seq,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_seq + i),*(end_time_seq + i),*(execution_time_seq + i),jitter_seq);      //Write all values calculated in file


			}

		}
	}
	printf("WCET = %lf\n",wcet_seq);
	printf("ACET = %lf\n",total_exec_time_seq/frame_count);
	printf("********************************JITTER ANALYSIS SEQUENCER END*********************************\n");
}



void print_image_capture_analysis()
{

	printf("********************************JITTER ANALYSIS IMAGE_CAPTURE START********************************\n");

	total_exec_time_img_capture = 0;

	fptr_S1 = fopen(filename_S1,"w+");

	fprintf(fptr_S1,"S1 Count,Start_Time(in sec),End_Time(in sec),Image_Capture_Time(in msec),Jitter(in msec)");

	/* Calculate execution time, WCET and average execution time */
	for(int i=0;i<frame_count;i++)
	{
		/* Calculate execution time of each iteration of image capture */
		*(execution_time_img_capture + i) = (*(end_time_img_capture + i) - *(start_time_img_capture + i))*SEC_TO_MSEC;

		if(i == 10)
		{
			wcet_img_capture = *(execution_time_img_capture + i);
		}

		if(wcet_img_capture < *(execution_time_img_capture + i))
		{
			wcet_img_capture = *(execution_time_img_capture + i);
		}

		/* Calculate total time of execution for image capture thread */
		total_exec_time_img_capture += *(execution_time_img_capture + i);

	}

	for(int i=0;i<frame_count;i++)
	{

		if(freq_img_capture == 1)
		{
			if(i == 0)
			{
				jitter_img_capture = 0;
				fprintf(fptr_S1,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_capture + i),*(end_time_img_capture + i),*(execution_time_img_capture + i),jitter_img_capture*SEC_TO_MSEC);
			}
			else
			{
				jitter_img_capture = (*(start_time_img_capture + i - 1) + delay_1hz.tv_sec) - (*(start_time_img_capture + i)) ;
				fprintf(fptr_S1,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_capture + i),*(end_time_img_capture + i),*(execution_time_img_capture + i),jitter_img_capture*SEC_TO_MSEC);      //Write all values calculated in file


			}
		}
		else if (freq_img_capture == 10)
		{
			if(i == 0)
			{
				jitter_img_capture = 0;
				fprintf(fptr_S1,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_capture + i),*(end_time_img_capture + i),*(execution_time_img_capture + i),jitter_img_capture);
			}
			else
			{
				jitter_img_capture = (*(start_time_img_capture + i - 1) + (delay_10hz.tv_nsec/NSEC_PER_SEC)) - (*(start_time_img_capture + i)) ;
				fprintf(fptr_S1,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_capture + i),*(end_time_img_capture + i),*(execution_time_img_capture + i),jitter_img_capture);      //Write all values calculated in file


			}

		}
	}
	printf("WCET = %lf\n",wcet_img_capture);
	printf("ACET = %lf\n",total_exec_time_img_capture/frame_count);
	printf("********************************JITTER ANALYSIS IMAGE_CAPTURE END********************************\n");
}


void print_image_dump_analysis()
{

	printf("********************************JITTER ANALYSIS IMAGE_DUMP START********************************\n");

	total_exec_time_img_dump = 0;

	fptr_S2 = fopen(filename_S2,"w+");

	fprintf(fptr_S2,"S2 Count,Start_Time(in sec),End_Time(in sec),Image_Dump_Time(in msec),Jitter(in sec)");

	/* Calculate execution time, WCET and average execution time */
	for(int i=0;i<frame_count;i++)
	{
		/* Calculate execution time of each iteration of image capture */
		*(execution_time_img_dump + i) = (*(end_time_img_dump + i) - *(start_time_img_dump + i))*SEC_TO_MSEC;

		if(i == 10)
		{
			wcet_img_dump = *(execution_time_img_dump + i);
		}

		if(wcet_img_dump < *(execution_time_img_dump + i))
		{
			wcet_img_dump = *(execution_time_img_dump + i);
		}

		/* Calculate total time of execution for image capture thread */
		total_exec_time_img_dump += *(execution_time_img_dump + i);

	}

	for(int i=0;i<frame_count;i++)
	{

		if(freq_img_capture == 1)
		{
			if(i == 0)
			{
				jitter_img_dump = 0;
				fprintf(fptr_S2,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_dump + i),*(end_time_img_dump + i),*(execution_time_img_dump + i),jitter_img_dump*SEC_TO_MSEC);
			}
			else
			{
				jitter_img_dump = (*(start_time_img_dump + i - 1) + delay_1hz.tv_sec) - (*(start_time_img_dump + i)) ;
				fprintf(fptr_S2,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_dump + i),*(end_time_img_dump + i),*(execution_time_img_dump + i),jitter_img_dump*SEC_TO_MSEC);      //Write all values calculated in file


			}
		}
		else if (freq_img_capture == 10)
		{
			if(i == 0)
			{
				jitter_img_dump = 0;
				fprintf(fptr_S2,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_dump + i),*(end_time_img_dump + i),*(execution_time_img_dump + i),jitter_img_dump);
			}
			else
			{
				jitter_img_dump = (*(start_time_img_dump + i - 1) + (delay_10hz.tv_nsec/NSEC_PER_SEC)) - (*(start_time_img_dump + i)) ;
				fprintf(fptr_S2,"\n%d,%lf,%lf,%lf,%lf",i,*(start_time_img_dump + i),*(end_time_img_dump + i),*(execution_time_img_dump + i),jitter_img_dump);      //Write all values calculated in file


			}

		}
	}
	printf("WCET = %lf\n",wcet_img_dump);
	printf("ACET = %lf\n",total_exec_time_img_dump/frame_count);

	printf("*********************************JITTER ANALYSIS IMAGE_DUMP END*********************************\n");
}
