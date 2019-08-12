#ifndef IMAGE_SEND_H
#define IMAGE_SEND_H


/*@Author       : Om Raheja
 *@File Name    : image_send.h
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief        : This file includes all the standard c library header files.
 * */

/*@Function Name: Service_3
 *@Brief        : Sends images via sockets to the host machine.
 *@Param in     : void *threadp
 *@Return       : void
 * */
void *Service_3(void *threadp);


#endif
