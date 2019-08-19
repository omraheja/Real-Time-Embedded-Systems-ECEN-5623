#ifndef CLIENT_H
#define CLIENT_H

/*@Author       : Om Raheja
 *@File Name    : client.h
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief        : This file includes all the standard c library header files.
 * */



/*@Function Name: client_send
 *@Brief        : Implements the send() command to send the required number of bytes of data 
 		: via sockets.
 *@Param in     : char ppm_dumpname[] (image name)
 *@Return       : void
 * */
void client_send(char ppm_dumpname[]);


/*@Function Name: init_client
 *@Brief        : initializes the client to send images via socket to the server (host machine)
 *@Param in     : void
 *@Return       : int
 * */
int init_client();

#endif
