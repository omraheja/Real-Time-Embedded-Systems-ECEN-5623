#ifndef IMAGE_SEND_H
#define IMAGE_SEND_H

/*@Author       : Om Raheja
 *@File Name    : image_send.h
 *@Date         : 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Contains function prototypes for image capture.
 */


/*@Function Name: Service_3
 *@Brief        : This is the third thread in which image transfer over ethernet takes place.
 *@Param in [1] : *threadp
 *@Return       : void
 * */
void *Service_3(void *threadp);


#endif
