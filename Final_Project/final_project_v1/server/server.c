/* Server*/
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 8080

int tag = 0;
int total_image_size;
char image_received[] = "imag00000000.ppm";

int main(int argc, char const *argv[]) 
{ 
#if 0
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 

	FILE *file_ptr;
	unsigned char buffer[921650];
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

#endif

	int server_fd,new_socket,valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	unsigned char buffer[921650];
	FILE *file_ptr;
	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	// Forcefully attaching socket to the port 8080
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
			//valread = read(new_socket,(char *)&buffer, 921650); 
			valread = recv(new_socket,(char *)&buffer, 921650,0); 
			total_image_size += valread;
			//printf("Bytes read = %d\n",valread);
			if(i == 0)
			{
				snprintf(&image_received[4], 9, "%08d", tag);
				strncat(&image_received[12], ".ppm", 5);

				file_ptr = fopen(image_received,"w");
				i++;
			}
			int write_size = fwrite(buffer,1,valread,file_ptr);
			//printf("Bytes written = %d\n",write_size);
		}while(total_image_size < 921650);

		fclose(file_ptr);
		i = 0;
	}



	return 0; 
} 

