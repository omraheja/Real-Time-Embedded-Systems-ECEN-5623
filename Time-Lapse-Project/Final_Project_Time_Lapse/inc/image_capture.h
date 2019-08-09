#ifndef IMAGE_CAPTURE_H
#define IMAGE_CAPTURE_H

/*@Author       : Om Raheja
 *@File Name    : image_capture.h
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Contains function prototypes for image capture.
 */


/*@Function Name: mainloop
 *@Brief        : Calls read_frame() to start image acquisition
 *@Param in     : void
 *@Return       : void
 * */
void mainloop(void);



/*@Function Name: read_frame
 *@Brief        : reads the frame from the camera and calls the prcess_image function for further processing.
 *@Param in     : void
 *@Return       : 1 (SUCCESS)
 * */
int read_frame(void);



/*@Function Name: process_image
 *@Brief        : Processes the image and converts it into RGB format.
 *@Param in [1] : const  void *p (Buffer which has to be processed)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Return       : void
 * */
void process_image(const void *p, int size);



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
void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b);




/*@Function Name: Service_1
 *@Brief        : This is the first thread in which image acquisition is carries out.
 *@Param in [1] : *threadp
 *@Return       : void
 * */
void *Service_1(void *threadp);


#endif
