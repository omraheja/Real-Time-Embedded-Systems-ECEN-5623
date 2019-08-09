/*@Author       : Om Raheja
 *@File Name    : v4l2_driver.c
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Contains function definition of function necessary for capturing image from 
 * 		  the camera.
 */


/* User defined headers */
#include "../inc/v4l2_driver.h"

/* Standard C library headers */
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Defines */
#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* Structures */
struct buffer
{
	void   *start;
	size_t  length;
};

/* External global variables */
extern int fd;
extern unsigned int n_buffers;
extern struct buffer *buffers;
extern char *dev_name;  //To store device name                          [command line argument]
extern int  HRES;       //To store Horizontal resolution                [command line argument]
extern int  VRES;       //To store Vertical resolution                  [command line argument]
extern int force_format;
extern struct v4l2_format fmt; //Format is used by a number of functions, so made as a file global


void open_device(void)
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


void init_device(void)
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



	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		fprintf(stderr, "%s does not support streaming i/o\n",
				dev_name);
		exit(EXIT_FAILURE);
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


	init_mmap();

}


void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

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

}


void stop_capturing(void)
{
	enum v4l2_buf_type type;



	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		errno_exit("VIDIOC_STREAMOFF");

}


void uninit_device(void)
{
	int i;


	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
			errno_exit("munmap");




	free(buffers);
}


void close_device(void)
{
	if (-1 == close(fd))
		errno_exit("close");

	fd = -1;
}



int xioctl(int fh, int request, void *arg)
{
	int r;

	do
	{
		r = ioctl(fh, request, arg);

	} while (-1 == r && EINTR == errno);

	return r;
}



void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}


void init_read(unsigned int buffer_size)
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


void init_mmap(void)
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


void init_userp(unsigned int buffer_size)
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
