/*@Author	: 
 *@Reference	: The code for image capturing and frame acquisition has been referenced from
 *		  the official website of V4L2(Video for Linux). The v4l2 driver code can also
 *		  be found on Dr.Sam Siewert's course website page.
 *		  
 *		  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  		  grabber NTSC cameras to acquire digital video from a source,
 *  		  time-stamp each frame acquired, save to a PGM or PPM file.
 *  		  
 *  		  The original code adapted was open source from V4L2 API and had the
 *  		  following use and incorporation policy:
 *  		  This program can be used and distributed without restrictions.
 *  		  This program is provided with the V4L2 API see http://linuxtv.org/docs.php for more information

 *@Date		: 
 *@Board used	: NVIDIA's Jetson Nano running Linux.
 *@Toiols	: Compiler:gcc ; Editor: Vim ; Camera: LogiTech C270
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
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <linux/videodev2.h>
#include <sched.h>
#include <time.h>



/************************************ DEFINES AND MACROS **********************************/
#define NUM_THREADS	(4)
#define NUM_CPU_CORES	(1)
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define OK 	(1)
#define K 	(4.0)
#define USEC_PER_MSEC (1000)
#define NSEC_PER_SEC (1000000000)
#define ERROR 	(-1)
#define TRUE	(1)
#define FALSE	(0)

/******************************** STRUCTURES AND ENUMERATIONS ******************************/
typedef struct
{
    int threadIdx;
    int MajorPeriods;
    int frame_count;
    unsigned long long sequencePeriods;
} threadParams_t;


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

/***************************************************************************************/


/************************************ GLOBAL VARIABLES *********************************/
static char *dev_name;		//To store device name				[command line argument]
static char hres_str[3];	//To store horizontal resolution as a string	[command line argument]
static char vres_str[3];	//To store vertical resolution as a string	[command line argument]
static int  frame_count;	//To store the number of frames to be captured  [command line argument]
static int  HRES;		//To store Horizontal resolution		[command line argument]
static int  VRES;		//To store Vertical resolution			[command line argument]

static unsigned int n_buffers;
struct buffer       *buffers;
static int          fd = -1;
static int          out_buf;
static int          force_format=1;

unsigned int  framecnt=0;
unsigned char bigbuffer[(1280*960*3)];

struct v4l2_buffer buf;
static struct v4l2_format fmt;	//Format is used by a number of functions, so made as a file global


//static enum io_method io = IO_METHOD_USERPTR;
//static enum io_method io = IO_METHOD_READ;
static enum io_method io = IO_METHOD_MMAP;

/* PPM header */
char ppm_header[50];
char ppm_dumpname[]="Test00000000.pgm";


/* Semaphores */
int abortTest	= FALSE;
int abortS1	= FALSE;
int abortS2	= FALSE;
int abortS3	= FALSE;
int abortS4	= FALSE;

sem_t semS1,semS2,semS3,semS4;



















/********************************************************************************/

/******************************** FUNCTION PROTOTYPES ******************************/
void print_scheduler(void);
static void open_device(void);
static void init_device(void);
static void start_capturing(void);
static void stop_capturing(void);
static void uninit_device(void);
static void close_device(void);
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t);
static int xioctl(int fh, int request, void *arg);
static void errno_exit(const char *s);
static void init_read(unsigned int buffer_size);
static void init_mmap(void);
static void init_userp(unsigned int buffer_size);
static void usage(FILE *fp, int argc, char **argv);

void *Sequencer(void *threadp);
void *Service_1(void *threadp);
void *Service_2(void *threadp);
void *Service_3(void *threadp);
void *Service_4(void *threadp);




/**********************************************************************************/


/******************************* FUNCTION 1 *****************************************/
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
           printf("Pthread Policy is SCHED_OTHER\n");
	   break;
     
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n");
	   exit(-1);
           break;
     
     default:
	   printf("Pthread Policy is UNKNOWN\n");
	   exit(-1);
   }

}


/******************************* FUNCTION 2 *****************************************/
/*@Function Name: open_device
 *@Brief        : Checks the status of the device and opens the device for use.
 *@Param in     : void
 *@Return       : void
 * */
static void open_device(void)
{
        struct stat st;

	/*@Function Name: stat()
	 *@Brief	: This function returns information about a file in the 
	 *		  structure 'st'.
	 *@Param in	: dev_name (/dev/video0) -> pathname of device whose
	 *		  information is being extracted.
	 *@Return	:  0 on success
	 * 		: -1 on failure
	 * */
        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }


	/*@Macro Name	: S_ISCHR()
	 *@Brief	: Checks if the device is a character special file or not 
	 *@Param in	: st.st_mode (This field contains the file type and mode)
	 *@Return	: 1 if file is a character special file
	 *		: 0 if file is NOT a character special file
	 * */
        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }


        /*@Function Name: open()
         *@Brief        : System call to open the file specified by dev_name which is nothing but
	 *		  the pathname. 
	 *@Param in [1]	: pathname
	 *@Param in [2]	: file access mode
	 *@Return       : "File descriptor" on success
         *              : -1 if an error occured
         * */
        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}


/******************************* FUNCTION 3 *****************************************/
/*@Function Name: init_device
 *@Brief        : Initializes the camera and sets its capabilities.
 *@Param in     : void
 *@Return       : void
 * */
static void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;



    /*@Function Name: xioctl()
     *@Brief	    : Wrapper function over ioctl(). ioctl() is used to manipulate
     *		      device parameters of special files.
     *@Param in [1] : file descriptor
     *@Param in [2] : VIDIOC_QUERYCAP to query the capture.
     *@Param in [3] : 'cap' structure of type v4l2_capability
     *@Return       :  0 on success
     *		    : -1 on failure
     **/
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


/******************************* FUNCTION 4 *****************************************/
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


/******************************* FUNCTION 5 *****************************************/
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


/******************************* FUNCTION 6 *****************************************/
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


/******************************* FUNCTION 7 *****************************************/
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


/******************************* FUNCTION 8 *****************************************/
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



/******************************* FUNCTION 9 *****************************************/
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


/******************************* FUNCTION 10 ****************************************/
/*@Function Name: errno_exit
 *@Param in [1] : const char *s
 *@Return       : void
 * */
static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}


/******************************* FUNCTION 11 ****************************************/
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


/******************************* FUNCTION 12 ****************************************/
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


/******************************* FUNCTION 13 ****************************************/
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


/******************************* FUNCTION 14 ****************************************/
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


/******************************* FUNCTION 15 ****************************************/
void *Sequencer(void *threadp)
{
    struct timeval current_time_val;
    struct timespec delay_time = {1,0}; // delay for 33.33 msec, 30 Hz
    struct timespec remaining_time;
    double current_time;
    double residual;
    int rc, delay_cnt=0;
    unsigned long long seqCnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

//    gettimeofday(&current_time_val, (struct timezone *)0);
//    syslog(LOG_CRIT, "Sequencer thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
//    printf("Sequencer thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    do
    {
        delay_cnt=0; residual=0.0;

        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Sequencer thread prior to delay @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        do
        {
            rc=nanosleep(&delay_time, &remaining_time);

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
        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Sequencer cycle %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);


        if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);


        // Release each service at a sub-rate of the generic sequencer rate

        // Servcie_1 = RT_MAX-1	@ 1 Hz
        if((seqCnt % 1) == 0) sem_post(&semS1);

        // Service_2 = RT_MAX-2	@ 1 Hz
        if((seqCnt % 1) == 0) sem_post(&semS2);

        // Service_3 = RT_MAX-3	@ 1 Hz
        if((seqCnt % 1) == 0) sem_post(&semS3);


    } while(!abortTest && (seqCnt < frame_count));//   while(!abortTest && (seqCnt < threadParams->sequencePeriods));

    sem_post(&semS1); 
    sem_post(&semS2); 
    sem_post(&semS3);
    abortS1=TRUE;
    abortS2=TRUE;
    abortS3=TRUE;

    pthread_exit((void *)0);
}


/******************************* FUNCTION 16 ****************************************/
void *Service_1(void *threadp)
{
    struct timeval current_time_val;
    double current_time;
    unsigned long long S1Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    struct timespec S1_start_time;
    struct timespec S1_end_time;

    //gettimeofday(&current_time_val, (struct timezone *)0);
    //syslog(LOG_CRIT, "Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    //printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS1)
    {
        sem_wait(&semS1);
        S1Cnt++;
	clock_gettime(CLOCK_REALTIME, &S1_start_time);
	printf("S1 Count: %d\t S1 Time: %lf seconds\n",S1Cnt,((double)S1_start_time.tv_sec + (double)((S1_start_time.tv_nsec)/(double)1000000000)));
        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Frame Sampler release %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    }

    pthread_exit((void *)0);
}

/******************************* FUNCTION 17 ****************************************/
void *Service_2(void *threadp)
{
    struct timeval current_time_val;
    double current_time;
    unsigned long long S2Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    struct timespec S2_start_time;
    struct timespec S2_end_time;

    //gettimeofday(&current_time_val, (struct timezone *)0);
    //syslog(LOG_CRIT, "Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    //printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS2)
    {
        sem_wait(&semS2);
        S2Cnt++;

	clock_gettime(CLOCK_REALTIME, &S2_start_time);
        printf("S2 Count: %d\t S2 Time: %lf seconds\n",S2Cnt,((double)S2_start_time.tv_sec + (double)((S2_start_time.tv_nsec)/(double)1000000000)));

        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Frame Sampler release %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    }

    pthread_exit((void *)0);
}

/******************************* FUNCTION 18 ****************************************/
void *Service_3(void *threadp)
{
    struct timeval current_time_val;
    double current_time;
    unsigned long long S3Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    struct timespec S3_start_time;
    struct timespec S3_end_time;

    //gettimeofday(&current_time_val, (struct timezone *)0);
    //syslog(LOG_CRIT, "Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    //printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS3)
    {
        sem_wait(&semS3);
        S3Cnt++;
	clock_gettime(CLOCK_REALTIME, &S3_start_time);
        printf("S3 Count: %d\t S3 Time: %lf seconds\n",S3Cnt,((double)S3_start_time.tv_sec + (double)((S3_start_time.tv_nsec)/(double)1000000000)));


        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Frame Sampler release %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    }

    pthread_exit((void *)0);
}

/******************************* FUNCTION 19 ****************************************/


/******************************* FUNCTION 20 ****************************************/


/******************************* FUNCTION 21 ****************************************/


/******************************* FUNCTION 21 ****************************************/












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
	
	printf("**********STARTING TIME LAPSE PROGRAM**********\n");
	
	if(argc > 1)
	{
		dev_name = argv[1];
		frame_count = atoi(argv[2]);
		HRES = atoi(argv[3]);
		VRES = atoi(argv[4]);

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


    	/* initialize the sequencer semaphores */
    	if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    	if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    	if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
    	if (sem_init (&semS4, 0, 0)) { printf ("Failed to initialize S4 semaphore\n"); exit (-1); }


	for (;;)
    	{
		int idx;
        	int c;

        	c = getopt_long(argc, argv,
                	    short_options, long_options, &idx);

        	if (-1 == c)
		{
			break;
		}

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



	/* Create Service threads which will block awaiting release for: */
    	
	// Service_1 = RT_MAX-1	@ 3 Hz
	rt_param[1].sched_priority=rt_max_prio-1;
	pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
	rc=pthread_create(&threads[1], &rt_sched_attr[1], Service_1, (void *)&(threadParams[1]));
	if(rc < 0)
		perror("pthread_create for service 2");
	else
		printf("pthread_create successful for service 2\n");

	// Service_2 = RT_MAX-2 @ 2 Hz
        rt_param[2].sched_priority=rt_max_prio-2;
        pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
        rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2, (void *)&(threadParams[2]));
        if(rc < 0)
                perror("pthread_create for service 2");
        else
                printf("pthread_create successful for service 2\n");

	// Service_3 = RT_MAX-3 @ 1 Hz
        rt_param[3].sched_priority=rt_max_prio-3;
        pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
        rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3, (void *)&(threadParams[3]));
        if(rc < 0)
                perror("pthread_create for service 2");
        else
                printf("pthread_create successful for service 2\n");





	// Note that the sleep is not necessary of RT service threads are created wtih
	// correct POSIX SCHED_FIFO priorities compared to non-RT priority of this main
	// program.
	//
	//usleep(1000000);

	// Create Sequencer thread, which like a cyclic executive, is highest prio
	printf("Start sequencer\n");
	threadParams[0].sequencePeriods=900;

	// Sequencer = RT_MAX	@ 30 Hz
    	//
    	rt_param[0].sched_priority = rt_max_prio;
    	pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    	rc=pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, (void *)&(threadParams[0]));
    	if(rc < 0)
    	    perror("pthread_create for sequencer service 0");
    	else
    	    printf("pthread_create successful for sequeencer service 0\n");




	for(int i=0;i<NUM_THREADS;i++)
		pthread_join(threads[i], NULL);
	
	printf("\nTEST COMPLETE\n");



	



	return 0;
}
