/*@Author	: Om Raheja
 *@Reference	: This code has been taken as a reference for the assignment for the course
 *		  Real-Time-Embedded-Systems (ECEN 5623).The driver for this code has been
 *		  refernced from the official website of V4L2. The code can also be found 
 *		  on Dr.Sam Siewert's course website page.
 *		  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  		  grabber NTSC cameras to acquire digital video from a source,
 *  		  time-stamp each frame acquired, save to a PGM or PPM file.
 *  		  The original code adapted was open source from V4L2 API and had the
 *  		  following use and incorporation policy:
 *  		  This program can be used and distributed without restrictions.
 *  		  This program is provided with the V4L2 API see http://linuxtv.org/docs.php for more information

 *@Date		: 19th July 2019
 *@Board used	: NVIDIA's Jetson Nano running Linux.
 *@Tools	: Compiler:gcc ; Editor: Vim
 * */

/* NOTE: Place #define _GNU_SOURCE before all the #include (s) and specially before #include <sched.h> in order
 * to avoid any error related to undefined reference to CPU_ZERO, CPU_SET, CPU_COUNT macros.
 * Reference: https://stackoverflow.com/questions/24034631/error-message-undefined-reference-for-cpu-zero
 */
/* Standard C library Headers */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <linux/videodev2.h>
#include <sched.h>
#include <time.h>


/* Defines, Global structures, Global Instances and Global variables */
#define NUM_THREADS	(1)
#define NUM_CPU_CORES	(1)

#define DEADLINE_MARGIN_BNW	(1.2)
#define DEADLINE_MARGIN_SHARPEN	(1.2)
#define DEADLINE_MARGIN_GRAY	(1.2)

typedef double FLOAT;
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define OK 1
#define K 4.0
#define NSEC_PER_SEC (1000000000)
#define ERROR -1

FLOAT PSF[9] = {-K/8.0, -K/8.0, -K/8.0, -K/8.0, K+1.0, -K/8.0, -K/8.0, -K/8.0, -K/8.0};

static char *dev_name;
static int frame_count;
static char *transformation;
static int HRES;
static int VRES;

struct timespec frame_rate_start =  {0, 0};           //To measure average frame rate
struct timespec frame_rate_end   =  {0, 0};
struct timespec frame_rate       =  {0, 0};

struct timespec EachFrameStart =  {0, 0};           //To measure average frame rate
struct timespec EachFrameEnd   =  {0, 0};
struct timespec EachFrameTime  =  {0, 0};


typedef struct
{
    int threadIdx;
    int MajorPeriods;
} threadParams_t;


/* Format is used by a number of functions, so made as a file global */
static struct v4l2_format fmt;

enum io_method
{
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer
{
        void   *start;
        size_t  length;
};

static char            *dev_name;
//static enum io_method   io = IO_METHOD_USERPTR;
//static enum io_method   io = IO_METHOD_READ;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format=1;
static int              frame_count;
int			deadline_calc=0;	//Flag
FLOAT 			deadline;		//Store deadline
FLOAT			wcet_max;		//Store maximum time taken by any of the frames
FLOAT			wcet;			//Store instantaneous time for each frame
FLOAT			total_jitter=0;		//Store total jitter
FLOAT			positive_jitter=0;	//Stores positive jitter
FLOAT			negative_jitter=0;	//Stores negative jitter
FLOAT			jitter=0;		//Stores instantaneous jitter values
int			deadline_miss;		//Stores the number of deadlines missed

FILE *fptr;
char filename[] = "Jitter_Analysis.csv";

unsigned int framecnt=0;
unsigned char bigbuffer[(1280*960*3)];

struct v4l2_buffer buf;


//char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char ppm_header[50];
char ppm_dumpname[]="Test00000000.pgm";

//char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_header[50];
char pgm_dumpname[]="test00000000.pgm";



static const char short_options[] = "d:hmruofc:";
static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};


/* FUNCTION PROTOTYPES */
static void open_device(void);
static void usage(FILE *fp, int argc, char **argv);
void *Sharpen_Image_Transform(void *threadp);
void *BnW_Image_Transform(void *threadp);
void *GrayScale_Image_Transform(void *threadp);
static void errno_exit(const char *s);
static void close_device(void);
static void open_device(void);
void print_scheduler(void);
static void uninit_device(void);
static void init_device(void);
static void start_capturing(void);
static int xioctl(int fh, int request, void *arg);
static void init_read(unsigned int buffer_size);
static void init_mmap(void);
static void init_userp(unsigned int buffer_size);
static void stop_capturing(void);
static void mainloop(void);
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t);
static int read_frame(void);
static void process_image(const void *p, int size);
void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b);
void sharpen_image(const void *p, int size);
static void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);
static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);
void bnw_image(const void *p,int size);






/*@Function Name: bnw_image
 *@Brief	: Takes the buffer on which Black and White transformation has to be applied.
 		  Converts the image into Black and White by comparing each pixel value
		  with a predefined threshold.
 *@Param in [1]	: const  void *p (Buffer to be transformed)
 *@Param in [2] : int size (Size of the Buffer being passed for transformation)
 *@Return	: void	
 * */
void bnw_image(const void *p,int size)
{
	unsigned char *pptr = (unsigned char *)p;
	for(int i=0;i<size;i++)
	{
		if(pptr[i] < 50)
		{
			pptr[i] = 0;
		}
		else
		{
			pptr[i] = 200;
		}
	}
}


/*@Function Name: dump_pgm
 *@Brief        : Writes the pixel values into an image.
 *@Param in [1] : const  void *p (Buffer which has to be converted to an image)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Param in [3] : unsigned int tag (Tag for Header)
 *@Param in [4] : struct timespec *time (Used to calculate timestamp for the Image header)
 *@Return       : void
 * */
static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;
    snprintf(&pgm_dumpname[4], 9, "%08d", tag);
    strncat(&pgm_dumpname[12], ".pgm", 5);
    dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
    snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&pgm_header[14], " sec ", 5);
    snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    //strncat(&pgm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);
    written=write(dumpfd, pgm_header, sizeof(pgm_header));
    total=0;
    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);
    //printf("wrote %d bytes\n", total);
    close(dumpfd);
}






/*@Function Name: dump_ppm
 *@Brief        : Writes the pixel values into an image.
 *@Param in [1] : const  void *p (Buffer which has to be converted to an image)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Param in [3] : unsigned int tag (Tag for Header)
 *@Param in [4] : struct timespec *time (Used to calculate timestamp for the Image header)
 *@Return       : void
 * */
static void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;
    snprintf(&ppm_dumpname[4], 9, "%08d", tag);
    strncat(&ppm_dumpname[12], ".ppm", 5);
    dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
    snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&ppm_header[14], " sec ", 5);
    snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    //strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);
    written=write(dumpfd, ppm_header, sizeof(ppm_header));
    total=0;
    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);
    //printf("wrote %d bytes\n", total);
    close(dumpfd);
}







/* This is probably the most acceptable conversion from camera YUYV to RGB.
 * Wikipedia has a good discussion on the details of various conversions
 * and cites good references http://en.wikipedia.org/wiki/YUV Also http://www.fourcc.org/yuv.php
 * What's not clear without knowing more about the camera in question is how
 * often U & V are sampled compare to Y.
 * E.g. YUV444, which is equivalent to RGB, where both require 3 bytes for each pixel
 * YUV422, which we assume here, where there are 2 bytes for each pixel, with two Y 
 * samples for one U & V, or as the name implies, 4Y and 2 UV pairs YUV420, where 
 * for every 4 Ys, there is a single UV pair, 1.5 bytes for each pixel or 36 bytes for 24 pixels.
*/
/*@Function Name    : yuv2rgb
 *@Brief            : Converts image from YUYV format to RGBRGB format
 *@Param in [1,2,3] : int y,int u,int v
 *@Param in [4,5,6] : unsigned char* r, unsigned char*g, unsigned char* b
 *@Return           : void
 * */
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



/*@Function Name: sharpen_image
 *@Brief        : Takes the image as an input and shrapens its using the sharpening algorithm.
 *@Param in [1] : const  void *p (Buffer which has to be sharpened)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Return       : void
 * */
void sharpen_image(const void *p, int size)
{
    int i,j,newi;
    FLOAT temp;
    unsigned char R[VRES*HRES];
    unsigned char G[VRES*HRES];
    unsigned char B[VRES*HRES];
    unsigned char convR[VRES*HRES];
    unsigned char convG[VRES*HRES];
    unsigned char convB[VRES*HRES];

    for(i=0,newi=0; i<size; (i=i+3),newi++)
    {
        R[newi]=*((unsigned char *)(p+i));
        G[newi]=*((unsigned char *)(p+i+1));
        B[newi]=*((unsigned char *)(p+i+2));
    }

    // Skip first and last row, no neighbors to convolve with
    for(i=1; i<((VRES)-1); i++)
    {
        // Skip first and last column, no neighbors to convolve with
        for(j=1; j<((HRES)-1); j++)
        {
            temp=0;
            temp += (PSF[0] * (FLOAT)R[((i-1)*HRES)+j-1]);
            temp += (PSF[1] * (FLOAT)R[((i-1)*HRES)+j]);
            temp += (PSF[2] * (FLOAT)R[((i-1)*HRES)+j+1]);
            temp += (PSF[3] * (FLOAT)R[((i)*HRES)+j-1]);
            temp += (PSF[4] * (FLOAT)R[((i)*HRES)+j]);
            temp += (PSF[5] * (FLOAT)R[((i)*HRES)+j+1]);
            temp += (PSF[6] * (FLOAT)R[((i+1)*HRES)+j-1]);
            temp += (PSF[7] * (FLOAT)R[((i+1)*HRES)+j]);
            temp += (PSF[8] * (FLOAT)R[((i+1)*HRES)+j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convR[(i*HRES)+j]=(unsigned char)temp;

            temp=0;
            temp += (PSF[0] * (FLOAT)G[((i-1)*HRES)+j-1]);
            temp += (PSF[1] * (FLOAT)G[((i-1)*HRES)+j]);
            temp += (PSF[2] * (FLOAT)G[((i-1)*HRES)+j+1]);
            temp += (PSF[3] * (FLOAT)G[((i)*HRES)+j-1]);
            temp += (PSF[4] * (FLOAT)G[((i)*HRES)+j]);
            temp += (PSF[5] * (FLOAT)G[((i)*HRES)+j+1]);
            temp += (PSF[6] * (FLOAT)G[((i+1)*HRES)+j-1]);
            temp += (PSF[7] * (FLOAT)G[((i+1)*HRES)+j]);
            temp += (PSF[8] * (FLOAT)G[((i+1)*HRES)+j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convG[(i*HRES)+j]=(unsigned char)temp;

            temp=0;
            temp += (PSF[0] * (FLOAT)B[((i-1)*HRES)+j-1]);
            temp += (PSF[1] * (FLOAT)B[((i-1)*HRES)+j]);
            temp += (PSF[2] * (FLOAT)B[((i-1)*HRES)+j+1]);
            temp += (PSF[3] * (FLOAT)B[((i)*HRES)+j-1]);
            temp += (PSF[4] * (FLOAT)B[((i)*HRES)+j]);
            temp += (PSF[5] * (FLOAT)B[((i)*HRES)+j+1]);
            temp += (PSF[6] * (FLOAT)B[((i+1)*HRES)+j-1]);
            temp += (PSF[7] * (FLOAT)B[((i+1)*HRES)+j]);
            temp += (PSF[8] * (FLOAT)B[((i+1)*HRES)+j+1]);
	  if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convB[(i*HRES)+j]=(unsigned char)temp;
        }
    }

    for(i=0,newi=0; newi<size; i++,(newi=newi+3))
    {
        *((unsigned char *)(p+newi))=convR[i];
        *((unsigned char *)(p+newi+1))=convG[i];
        *((unsigned char *)(p+newi+2))=convB[i];
    }

}


/*@Function Name: process_image
 *@Brief        : Processes and applies transforms to image based on command line inputs specifying
 		  which transform to perform.
 *@Param in [1] : const  void *p (Buffer which has to be processed)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Return       : void
 * */
static void process_image(const void *p, int size)
{
    int i, newi, newsize=0;
    struct timespec frame_time;
    int y_temp, y2_temp, u_temp, v_temp;
    unsigned char *pptr = (unsigned char *)p;

    // record when process was called
    clock_gettime(CLOCK_REALTIME, &frame_time);    

    framecnt++;
    //printf("frame %d: ", framecnt);

    // This just dumps the frame to a file now, but you could replace with whatever image
    // processing you wish.
    //

    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
    {
        printf("Dump graymap as-is size %d\n", size);
        //dump_pgm(p, size, framecnt, &frame_time);
    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
	    if(strcmp(transformation,"sharpen")==0)
	    {
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
		    sharpen_image(bigbuffer,((size*6)/4));
		    dump_ppm(bigbuffer, ((size*6)/4), framecnt, &frame_time);
	    }
	    else if(strcmp(transformation,"bnw")==0)
	    {
		    //printf("Dump YUYV converted to YY size %d\n", size);
		    
		    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
		    // We want Y, so YY which is 2 bytes
		    //
		    for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
		    {
			    // Y1=first byte and Y2=third byte
			    bigbuffer[newi]=pptr[i];
			    bigbuffer[newi+1]=pptr[i+2];
		    }
		    bnw_image(bigbuffer,(size/2));
		    dump_pgm(bigbuffer, (size/2), framecnt, &frame_time);
	    }
	    else if(strcmp(transformation,"grayscale")==0)
	    {
		    //printf("Dump YUYV converted to YY size %d\n", size);

                    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
                    // We want Y, so YY which is 2 bytes
                    //
                    for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
                    {
                            // Y1=first byte and Y2=third byte
                            bigbuffer[newi]=pptr[i];
                            bigbuffer[newi+1]=pptr[i+2];
                    }
                    dump_pgm(bigbuffer, (size/2), framecnt, &frame_time);
	    }

    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)
    {
        printf("Dump RGB as-is size %d\n", size);
        dump_ppm(p, size, framecnt, &frame_time);
    }
    else
    {
        printf("ERROR - unknown dump format\n");
    }

    fflush(stderr);
    //fprintf(stderr, ".");
    fflush(stdout);
}






/*@Function Name: read_frame
 *@Brief        : reads the frame from the camera and calls the prcess_image function for further processing.
 *@Param in 	: void
 *@Return       : 1 (SUCCESS)
 * */
static int read_frame(void)
{
    //struct v4l2_buffer buf;
    unsigned int i;

    switch (io)
    {

        case IO_METHOD_READ:
            if (-1 == read(fd, buffers[0].start, buffers[0].length))
            {
                switch (errno)
                {

                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("read");
                }
            }

            process_image(buffers[0].start, buffers[0].length);
            break;

        case IO_METHOD_MMAP:
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
            break;

        case IO_METHOD_USERPTR:
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

            for (i = 0; i < n_buffers; ++i)
                    if (buf.m.userptr == (unsigned long)buffers[i].start
                        && buf.length == buffers[i].length)
                            break;

            assert(i < n_buffers);

            process_image((void *)buf.m.userptr, buf.bytesused);

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            break;
    }

    return 1;
}


/*@Function Name: delta_t
 *@Brief        : Takes two timespec structures as input and calculates the difference between them.
 *@Param in [1] : struct timespec *stop [Stores stop time]
 *@Param in [2] : struct timespec *start [Stores start time]
 *@Param in [3] : struct timespec *delta_t [Stores difference between stop time and start time]
 *@Return       : 1 (SUCCESS)
 * */
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  else
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  return(OK);
}



/*@Function Name: mainloop
 *@Brief        : Reads the frame, calculates WCETs for each frame and for the entire
 		  frame set, assigns deadline and checks for any deadline misses.
 *@Param in 	: void
 *@Return       : void
 * */
static void mainloop(void)
{
    unsigned int count;
    int frame_number=0;
    struct timespec read_delay;
    struct timespec time_error;
    double avg_wcet=0;
    read_delay.tv_sec=0;
    read_delay.tv_nsec=30000;

    count = frame_count;

    clock_gettime(CLOCK_REALTIME, &frame_rate_start);
    while (count > 0)
    {	
        for (;;)
        {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 4;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r)
            {
                if (EINTR == errno)
                    continue;
                errno_exit("select");
            }

            if (0 == r)
            {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            } 
	    
	    clock_gettime(CLOCK_REALTIME,&EachFrameStart);
            if (read_frame())
            {
                if(nanosleep(&read_delay, &time_error) != 0)
                    perror("nanosleep");
                else
                    //printf("time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n", time_error.tv_sec, time_error.tv_nsec);
		frame_number++;
                count--;
                break;
            }
	    

	    /* EAGAIN - continue select loop unless count done. */
            if(count <= 0) break;
        }
	clock_gettime(CLOCK_REALTIME,&EachFrameEnd);
	delta_t(&EachFrameEnd, &EachFrameStart, &EachFrameTime);
	wcet = ((double)EachFrameTime.tv_sec + (double)(((double)(EachFrameTime.tv_nsec))/((double)1000000000)));
	
	//printf("WCET[Frame %d] = %lf\n",frame_number,wcet);

	if(!deadline_calc)
	{
		avg_wcet += wcet;
		if(wcet_max < wcet)
		{
			wcet_max = wcet;
		}
		if(strcmp(transformation,"sharpen")==0)
		{
			deadline = (((double)avg_wcet)/((double)frame_count))*DEADLINE_MARGIN_SHARPEN;
			
		}
		else if(strcmp(transformation,"bnw")==0)
		{
			deadline = (((double)avg_wcet)/((double)frame_count))*DEADLINE_MARGIN_BNW;

		}
		else if(strcmp(transformation,"grayscale")==0)
		{
			deadline = (((double)avg_wcet)/((double)frame_count))*DEADLINE_MARGIN_GRAY;
		}
	}


	if(deadline_calc)
	{
		jitter = wcet - deadline;
		if(jitter > 0)
		{
			positive_jitter += jitter;
		//	printf("Positive Jitter = %lf\n",(jitter));
			deadline_miss++;
			printf("/*************Deadline missed : Frame %d******************/\n",frame_number);
		}
		else if(jitter < 0)
		{
			negative_jitter += jitter;
		//	printf("Negative Jitter = %lf\n",(jitter));
		}

		total_jitter = positive_jitter + negative_jitter;
		if(jitter > 0)
		{
			fprintf(fptr,"\n%d,%lf,0,%lf",frame_number,jitter,total_jitter);
		}
		if(jitter < 0)
		{
			fprintf(fptr,"\n%d,0,%lf,%lf",frame_number,jitter,total_jitter);
		}

	}
	
        if(count <= 0) break;
    }

    clock_gettime(CLOCK_REALTIME, &frame_rate_end);
    delta_t(&frame_rate_end, &frame_rate_start, &frame_rate);
    if(!deadline_calc)
    {
    	printf("Total Time Elapsed: %lf seconds\n",((double)frame_rate.tv_sec + (double)((frame_rate.tv_nsec)/(double)1000000000)));
    	printf("Frame Rate in Hz for a Resolution of %dx%d = %lf\n",HRES,VRES,(((double)frame_count)/((double)frame_rate.tv_sec + (double)((frame_rate.tv_nsec)/(double)1000000000))));
    	printf("AVG WCET 		= %lf seconds\n",((double)avg_wcet)/((double)frame_count));
	printf("Deadline 		= %lf seconds\n",deadline);
    }

    if(deadline_calc)
    {
	    printf("Positive Jitter		= %lf\n",positive_jitter);
	    printf("Negative Jitter		= %lf\n",negative_jitter);
	    printf("Total Jitter		= %lf\n",total_jitter);
	    printf("Total deadlines missed	= %d\n",deadline_miss);
    }
}


/*@Function Name: init_userp
 *@Param in [1] : unsigned int buffer_size
 *@Return       : void
 * */
static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}




/*@Function Name: init_mmap
 *@Param in	: void
 *@Return       : void
 * */
static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 6;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) 
        {
                if (EINVAL == errno) 
                {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);
                } else 
                {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) 
        {
                fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;
                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}


/*@Function Name: init_read
 *@Param in [1] : unsigned int buffer_size
 *@Return       : void
 * */
static void init_read(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));

        if (!buffers) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) 
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}




/*@Function Name: xioctl
 *@Brief        : wrapper for ioctl. Helps to change special files.
 *@Param in [1] : int fh
 *@Param in [2] : int request
 *@Param in [3] : void *arg
 *@Return       : int r;
 * */
static int xioctl(int fh, int request, void *arg)
{
        int r;

        do 
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}






/*@Function Name: start_capturing
 *@Brief        : Sets attributes required for camera to start capturing images
 *@Param in     : void
 *@Return       : void
 * */
static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) 
        {

        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i) 
                {
                        printf("allocated buffer %d\n", i);
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}


/*@Function Name: init_device
 *@Brief        : Initializes the camera and sets its capabilities
 *@Param in     : void
 *@Return       : void
 * */
static void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",
                     dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
                errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n",
                 dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf(stderr, "%s does not support read i/o\n",
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                fprintf(stderr, "%s does not support streaming i/o\n",
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;
    }


    /* Select video input, video standard and tune here. */


    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                        break;
            }
        }

    }
    else
    {
        /* Errors ignored. */
    }


    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (force_format)
    {
        printf("FORCING FORMAT\n");
        fmt.fmt.pix.width       = HRES;
        fmt.fmt.pix.height      = VRES;

        // Specify the Pixel Coding Formate here

        // This one work for Logitech C200
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY;

        // Would be nice if camera supported
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

        //fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */
    }
    else
    {
        printf("ASSUMING FORMAT\n");
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                    errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

    switch (io)
    {
        case IO_METHOD_READ:
            init_read(fmt.fmt.pix.sizeimage);
            break;

        case IO_METHOD_MMAP:
            init_mmap();
            break;

        case IO_METHOD_USERPTR:
            init_userp(fmt.fmt.pix.sizeimage);
            break;
    }
}



/*@Function Name: uninit_device
 *@Brief        : Free all the bufer that were previously allocated for the application.
 *@Param in 	: void
 *@Return       : void
 * */
static void uninit_device(void)
{
int i;
        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}




/*@Function Name: usage
 *@Param in [1] : FILE *fp (file pointer)
 *@Param in [2] : int argc
 *@Param in [3] : char **argv
 *@Return       : void
 * */
static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to 640x480 GREY\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}


/*@Function Name: *Sharpen_Image_Transform
 *@Brief        : Thread which carries out sharpening of image
 *@Param in [1] : void *threadp (thread_id)
 *@Return       : void
 * */
void *Sharpen_Image_Transform(void *threadp)
{
	threadParams_t *threadParams = (threadParams_t *)threadp;
	printf("/***********************ANALYSIS*******************************/\n");
        printf("Transformation :Sharpening \n");
	printf("Please wait while analysis is being carried out................\n");
	mainloop();
	deadline_calc = 1;
	mainloop();

}


/*@Function Name: errno_exit
 *@Param in [1] : const char *s
 *@Return       : void
 * */
static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}


/*@Function Name: close_device
 *@Brief        : Close the File descriptor of the device being used
 *@Param in     : void
 *@Return       : void
 * */
static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

/*@Function Name: open_device
 *@Brief        : Checks the status of the device and open the device for use.
 *@Param in     : void
 *@Return       : void
 * */
static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }


        /* S_ISCHR is a macro which checks if the device is a character special file or not */
        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}


/*@Function Name: stop_capturing
 *@Brief        : Changes properties using xioctl to stop capturing the images
 *@Param in     : void
 *@Return       : void
 * */
static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}


/*@Function Name: print_scheduler
 *@Brief        : Prints the scheduling policy
 *@Param in     : void
 *@Return       : void
 * */
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

/* Global variables to store the resolution provided at run-time */
char hres_str[3];
char vres_str[3];



int main(int argc,char **argv)
{
	if(argc >1)
	{
		dev_name = argv[1];
		frame_count = atoi(argv[2]);
		transformation = "sharpen";
		HRES = atoi(argv[3]);
		VRES = atoi(argv[4]);
		
		strncpy(hres_str,argv[3],sizeof(hres_str));
		strncpy(vres_str,argv[4],sizeof(vres_str));
		printf("Frame count = %d\n",frame_count);
		printf("Transformation = %s\n",transformation);	
		printf("Resolution = %dx%d\n",HRES,VRES);
		sprintf(ppm_header,"P6\n#9999999999 sec 9999999999 msec \n%s %s\n255\n",hres_str,vres_str);
		sprintf(pgm_header,"P5\n#9999999999 sec 9999999999 msec \n%s %s\n255\n",hres_str,vres_str);
				
	}
	else
	{
		printf("USAGE: [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES]\n");
		exit(-1);
	}


	int rc,scope;
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


	/*Display number of processors and number of available processors */
        printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());


	/*Clears set, so that it contains no CPUs */
        CPU_ZERO(&allcpuset);

        /* Add CPU cpu to the set */
        for(int i=0; i < NUM_CPU_CORES; i++)
        {
                CPU_SET(i, &allcpuset);
        }

        printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));

	/* Find the range of max priority for the SCHED_FIFO scheduling policy */
        rt_max_prio = sched_get_priority_max(SCHED_FIFO);
        rt_min_prio = sched_get_priority_min(SCHED_FIFO);

	printf("rt_max_prio=%d\n", rt_max_prio);
        printf("rt_min_prio=%d\n", rt_min_prio);

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

	/*Assign cpu affinity to the thread so that the thread
         * is bounded to that CPU core only  */
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

	open_device();
        init_device();
        start_capturing();


	for (;;)
    	{
        int idx;
        int c;

        c = getopt_long(argc, argv,
                    short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c)
        {
            case 0: /* getopt_long() flag */
                break;

            case 'd':
                dev_name = optarg;
                break;

            case 'h':
                usage(stdout, argc, argv);
                exit(EXIT_SUCCESS);

            case 'm':
                io = IO_METHOD_MMAP;
                break;

            case 'r':
                io = IO_METHOD_READ;
                break;

            case 'u':
                io = IO_METHOD_USERPTR;
                break;

            case 'o':
                out_buf++;
                break;

            case 'f':
                force_format++;
                break;

            case 'c':
                errno = 0;
                frame_count = strtol(optarg, NULL, 0);
                if (errno)
                        errno_exit(optarg);
                break;

            default:
                usage(stderr, argc, argv);
                exit(EXIT_FAILURE);
        }
    }


	fptr = fopen(filename,"w+");
	fprintf(fptr,"Sr.No,Positive Jitter,Negative Jitter,Total Jitter");

	if(strcmp(transformation,"sharpen") == 0)
        {
                // Create Threads [Sharpening Image]
    		rc=pthread_create(&threads[0],             // pointer to thread descriptor
                      		&rt_sched_attr[0],         // use specific attributes
                      		//(void *)0,               // default attributes
                      		Sharpen_Image_Transform,   // thread function entry point
                      		(void *)&(threadParams[0]) // parameters to pass in
                     		);
        }
        else
        {
                printf("Invalid Transformation\n");
                printf("USAGE: [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES]\n");
        }


	 /* Wait until all threads finish their execution */
   	for(int i=0;i<NUM_THREADS;i++)
   	{
		pthread_join(threads[i], NULL);
	}


	stop_capturing();
    	uninit_device();
    	close_device();
    	fprintf(stderr, "\n");
	fclose(fptr);
    	return 0;


}
