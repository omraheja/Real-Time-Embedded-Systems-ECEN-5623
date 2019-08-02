/*@Author       : Om Raheja
 *@File Name    : image_capture.c 
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Reads the frames from the camera and converts it it into RGB format.
 */


/* User defined header files */
#include "../inc/image_capture.h"
#include "../inc/v4l2_driver.h"

/* Standard C library headers */
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>

/* Defines */
#define CLEAR(x) memset(&(x), 0, sizeof(x))


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


/* External global variables */
extern int abortS1;
extern sem_t semS1;
extern int fd;
extern unsigned char bigbuffer[(1280*960*3)];
extern struct v4l2_format fmt;		//Format is used by a number of functions, so made as a file global
extern unsigned int framecnt;
extern int size_of_image;
extern unsigned int n_buffers;
extern struct v4l2_buffer buf;
extern struct buffer *buffers;

/* Global variables */
struct timespec frame_time;

/* Service 1 [Image Capture] */
void *Service_1(void *threadp)
{
	struct timeval current_time_val;
	double current_time;
	unsigned long long S1Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	struct timespec S1_start_time;		//start time for each iteration until all frames are captured
	struct timespec S1_end_time;		//end time for each iteration until all frames are captured


	while(!abortS1)
	{
		sem_wait(&semS1);
		syslog(LOG_INFO,"IMAGE CAPTURE");
		clock_gettime(CLOCK_REALTIME, &S1_start_time);

		syslog(LOG_INFO,"S1 Count: %lld\t Image Capture start Time: %lf seconds\n",S1Cnt,((double)S1_start_time.tv_sec + (double)((S1_start_time.tv_nsec)/(double)1000000000)));
		S1Cnt++;

		mainloop();
		clock_gettime(CLOCK_REALTIME, &S1_end_time);
		syslog(LOG_INFO,"S1 Count: %lld\t Image Capture End Time: %lf seconds\n",S1Cnt,((double)S1_end_time.tv_sec + (double)((S1_end_time.tv_nsec)/(double)1000000000)));
	}

	pthread_exit((void *)0);
}



void mainloop(void)
{
	struct timespec read_delay;
	struct timespec time_error;

	read_delay.tv_sec=0;
	read_delay.tv_nsec=30000;

	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);

	if (-1 == r)
	{
		if (EINTR == errno)
			//continue;
			errno_exit("select");
	}

	if (0 == r)
	{
		fprintf(stderr, "select timeout\n");
		exit(EXIT_FAILURE);
	}

	if (read_frame())
	{
		if(nanosleep(&read_delay, &time_error) != 0)
			perror("nanosleep");
		else
		{
			//printf("time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n", time_error.tv_sec, time_error.tv_nsec);

		}
	}
}

int read_frame(void)
{

	unsigned int i;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		switch (errno)
		{
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, but drivers should only set for serious errors, although some set for
				   non-fatal errors too.
				   */
				return 0;


			default:
				printf("mmap failure\n");
				errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(buf.index < n_buffers);

	process_image(buffers[buf.index].start, buf.bytesused);

	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");

	return 1;
}





void process_image(const void *p, int size)
{
	int i, newi, newsize=0;
	int y_temp, y2_temp, u_temp, v_temp;
	unsigned char *pptr = (unsigned char *)p;

	size_of_image = size;	//store size of the image in a global variable
	// record when process was called
	clock_gettime(CLOCK_REALTIME, &frame_time);


	//printf("Dump YUYV converted to RGB size %d\n", size);
	// Pixels are YU and YV alternating, so YUYV which is 4 bytes
	// We want RGB, so RGBRGB which is 6 bytes
	//
	for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
	{
		y_temp=(int)pptr[i];
		u_temp=(int)pptr[i+1];
		y2_temp=(int)pptr[i+2];
		v_temp=(int)pptr[i+3];
		yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
		yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
	}




	fflush(stderr);
	//fprintf(stderr, ".");
	fflush(stdout);
}



void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int r1, g1, b1;

	// replaces floating point coefficients
	int c = y-16, d = u - 128, e = v - 128;

	// Conversion that avoids floating point
	r1 = (298 * c           + 409 * e + 128) >> 8;
	g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
	b1 = (298 * c + 516 * d           + 128) >> 8;

	// Computed values may need clipping.
	if (r1 > 255) r1 = 255;
	if (g1 > 255) g1 = 255;
	if (b1 > 255) b1 = 255;

	if (r1 < 0) r1 = 0;
	if (g1 < 0) g1 = 0;
	if (b1 < 0) b1 = 0;

	*r = r1 ;
	*g = g1 ;
	*b = b1 ;
}
