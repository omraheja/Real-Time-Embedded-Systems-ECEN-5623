/*@Author       : Om Raheja
 *@File Name    : server.c 
 *@Date         : 8/11/2019 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Receives the images from the client and stores it.
 *@References	: References for the client server code has been taken from "geeks_for_geeks".
 */


/* Standard C Library Headers */
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <syslog.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

/* Defines */
#define PORT 8080


/* Global variables */
int tag = 0;
int total_image_size;
char image_received[] = "imag00000000.ppm";

/* Main function */
int main(int argc, char const *argv[]) 
{ 
	int server_fd,new_socket,valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);	
	unsigned char buffer[921800];	//Buffer to store the received data
	FILE *file_ptr;			//File pointer 

	/* Creating socket file descriptor */
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* Forcefully attaching socket to the port 8080 */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	/* Forcefully attaching socket to the port 8080 */
	if (bind(server_fd, (struct sockaddr *)&address,
				sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	int i = 0;
	while(1)
	{
		total_image_size = 0;
		tag++;
		do{
			valread = recv(new_socket,(char *)&buffer, 921800,0); 
			total_image_size += valread;
			syslog(LOG_INFO,"Bytes read = %d\n",valread);
			if(i == 0)
			{
				snprintf(&image_received[4], 9, "%08d", tag);
				strncat(&image_received[12], ".ppm", 5);

				file_ptr = fopen(image_received,"w");
				i++;
			}
			int write_size = fwrite(buffer,1,valread,file_ptr);
			syslog(LOG_INFO,"%s Received\n",image_received);
			//printf("Bytes written = %d\n",write_size);
		}while(total_image_size < 921650);

		fclose(file_ptr);
		i = 0;
	}

	return 0; 
} 
