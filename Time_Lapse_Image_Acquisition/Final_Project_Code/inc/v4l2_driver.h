#ifndef V4L2_DRIVER_H
#define V4L2_DRIVER_H

/*@Author       : Om Raheja
 *@File Name    : v4l2_driver.h
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Contains function prototypes for function related to v4l2 driver.
 */


/*@Function Name: open_device
 *@Brief        : Checks the status of the device and opens the device for use.
 *@Param in     : void
 *@Return       : void
 * */
void open_device(void);


/*@Function Name: init_device
 *@Brief        : Initializes the camera and sets its capabilities.
 *@Param in     : void
 *@Return       : void
 * */
void init_device(void);


/*@Function Name: start_capturing
 *@Brief        : Sets attributes required for camera to start capturing images
 *@Param in     : void
 *@Return       : void
 * */
void start_capturing(void);


/*@Function Name: stop_capturing
 *@Brief        : Changes properties using xioctl to stop capturing the images
 *@Param in     : void
 *@Return       : void
 * */
void stop_capturing(void);


/*@Function Name: uninit_device
 *@Brief        : Free all the buffers that were previously allocated for the application.
 *@Param in     : void
 *@Return       : void
 * */
void uninit_device(void);


/*@Function Name: close_device
 *@Brief        : Close the File descriptor of the device being used
 *@Param in     : void
 *@Return       : void
 * */
void close_device(void);


/*@Function Name: errno_exit
 *@Param in [1] : const char *s
 *@Return       : void
 * */
void errno_exit(const char *s);



/*@Function Name: init_read
 *@Param in [1] : unsigned int buffer_size
 *@Return       : void
 * */
void init_read(unsigned int buffer_size);



/*@Function Name: init_mmap
 *@Param in     : void
 *@Return       : void
 * */
void init_mmap(void);


/*@Function Name: init_userp
 *@Param in [1] : unsigned int buffer_size
 *@Return       : void
 * */
void init_userp(unsigned int buffer_size);


/*@Function Name: xioctl
 *@Brief        : wrapper for ioctl. Helps to change special files.
 *@Param in [1] : int fh
 *@Param in [2] : int request
 *@Param in [3] : void *arg
 *@Return       : int r;
 * */
int xioctl(int fh, int request, void *arg);

#endif
