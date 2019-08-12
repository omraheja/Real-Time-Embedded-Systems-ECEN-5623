/*@Author       : Om Raheja
 *@File Name    : client.c 
 *@Date         : 8/11/2019 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Reads the frames from the camera and converts it it into RGB format.
 *@References	: References for the client server code has been taken from "geeks_for_geeks".
 */


/* Standard C library headers */
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <syslog.h>


/* User defined headers */
#include "../inc/client.h"


/* Defines */
#define PORT 8080


/* Global variables */
int sock = 0;
int valread;
int image_size;
int count = 0;
char send_buffer[921800];
struct sockaddr_in serv_addr;
static FILE *fptr;


int init_client()
{
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	return 0;

}

void client_send(char ppm_dumpname[])
{
	fptr = fopen(ppm_dumpname,"r");
	fseek(fptr,0,SEEK_END);
	image_size = ftell(fptr);
	
	fseek(fptr,0,SEEK_SET);
	int size = fread(send_buffer,1,sizeof(send_buffer),fptr);

	int rc = send(sock,(char *)&send_buffer,size,0);

	if(rc == -1)
	{
		syslog(LOG_INFO,"SOCKET SEND FAILED!");
		perror("Socket Send CLIENT:");
	}
	else
	{
		syslog(LOG_INFO,"Socket Send %d SUCCESSFUL = %d bytes",count,rc);
		count++;
	}
	fclose(fptr);

}
