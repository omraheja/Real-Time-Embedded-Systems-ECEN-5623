/*@Author       : Om Raheja
 *@File Name    : image_store.h 
 *@Date         : 8/11/2019 
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 */

/* Standard C library headers */
#include <stdio.h>
#include <time.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>

/* Image store service */
void *Service_2(void *threadp);

