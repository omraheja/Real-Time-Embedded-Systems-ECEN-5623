/*@Author       : Dr.Sam Siewert
 *@Modified by	: Om Raheja
 *@Date         : 3rd July 2019
 *@Reference	: This code has been modified by me (originally written
 *                by Dr.Sam Siewert)as a part of the assignment for the
 *                Real Time embedded systems course at the University of
 *                Colorado Boulder.
 *                The code was originally written for VxWorks and I modified
 *                it for the Linux environment running on the Jetson nano.
 *@Board used   : NVIDIA's JETSON NANO Development Board
 * */

/* Standard C Library Headers */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* Macros */
#define SNDRCV_MQ 		"/heap_mq"
#define RECEIVER_PRIORITY	(99)
#define SENDER_PRIORITY		(98)
#define ERROR			(-1)

/* Global variables */
struct mq_attr mq_attr;
static mqd_t mymq;
static char imagebuff[4096];

/* Sender Thread */
void *sender_thread()
{
	char buffer[sizeof(void *)+sizeof(int)];
	void *buffptr;
	int prio;
	int nbytes;
	int id = 999;

	while(1)
	{
		/* send malloc'd message with priority=30 */
		buffptr = (void *)malloc(sizeof(imagebuff));
		strcpy(buffptr,imagebuff);
		printf("Message to send = %s\n",(char *)buffptr);

		printf("Sending %ld bytes\n",sizeof(buffptr));

		memcpy(buffer, &buffptr, sizeof(void *));
		memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

		if((nbytes = mq_send(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)),30)) == ERROR)
		{
			perror("mq_send");
		}
		else
		{
			printf("send: message ptr 0x%p successfully sent\n",buffptr);
		}

		sleep(3);
	}
}

/* Receiver Thread */
void *receiver_thread()
{
	char buffer[sizeof(void *)+sizeof(int)];
  	void *buffptr;
  	int prio;
  	int nbytes;
  	int count = 0;
  	int id;

  	while(1)
	{

    		/* read oldest, highest priority msg from the message queue */
    		printf("Reading %ld bytes\n", sizeof(void *));
	
   		if((nbytes = mq_receive(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), &prio)) == ERROR)
    		{
      			perror("mq_receive");
    		}	
    		else
    		{
      			memcpy(&buffptr, buffer, sizeof(void *));
      			memcpy((void *)&id, &(buffer[sizeof(void *)]), sizeof(int));
      			printf("receive: ptr msg 0x%p received with priority = %d, length = %d, id = %d\n", buffptr, prio, nbytes, id);
      			printf("contents of ptr = \n%s\n", (char *)buffptr);
      			free(buffptr);
      			printf("heap space memory freed\n");
    		}
  	}	
}


int main()
{
	int i,j;
	pthread_t sender,receiver;

	/* instances for thread attributes */
        pthread_attr_t sender_attr,receiver_attr;

        /* instances for scheduling parameters */
        struct sched_param sender_sched_param, receiver_sched_param;

	 /* Initialize scheduling parameters */
        sender_sched_param.sched_priority = SENDER_PRIORITY;
        receiver_sched_param.sched_priority = RECEIVER_PRIORITY;

        /* Set pthread attributes */

        /* Initialize thread attributes */
        pthread_attr_init(&sender_attr);
        pthread_attr_init(&receiver_attr);

        /* Set scope of the threads */
        pthread_attr_setscope(&sender_attr,PTHREAD_SCOPE_SYSTEM);
        pthread_attr_setscope(&receiver_attr,PTHREAD_SCOPE_SYSTEM);

        /* Set Scheduling policy */
        pthread_attr_setschedpolicy(&sender_attr,SCHED_FIFO);
        pthread_attr_setschedpolicy(&receiver_attr,SCHED_FIFO);

        /* Set inherited scheduling policy */
        pthread_attr_setinheritsched(&sender_attr,PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setinheritsched(&receiver_attr,PTHREAD_EXPLICIT_SCHED);

        /* Set scheduling parameters */
        pthread_attr_setschedparam(&sender_attr,&sender_sched_param);
        pthread_attr_setschedparam(&receiver_attr,&receiver_sched_param);


	char pixel = 'A';

	for(i=0;i<4096;i+=64)
	{
		pixel = 'A';
		for(j=i;j<i+64;j++)
		{
			imagebuff[j] = (char)pixel++;
		}
		imagebuff[j-1] = '\n';
	}

	imagebuff[4095] = '\0';
	imagebuff[63] = '\0';

	printf("buffer =\n%s",imagebuff);

	/* setup common message q attributes */
	mq_attr.mq_maxmsg = 100;
	mq_attr.mq_msgsize = sizeof(void *) + sizeof(int);
	mq_attr.mq_flags = 0;

	mymq = mq_open(SNDRCV_MQ, (O_CREAT|O_RDWR), 0664, &mq_attr);

	if(mymq == (mqd_t)ERROR)
	{
		perror("mq_open");
	}

	/* Create threads */
        int rc = pthread_create(&receiver,&receiver_attr,receiver_thread,NULL);
        if(rc != 0)
        {
                printf("Receiver thread not created!\n");
        }
        else
        {
                printf("\nReceiver thread created\n");
        }

        rc = pthread_create(&sender,&sender_attr,sender_thread,NULL);
        if(rc !=0)
        {
                printf("Sender thread not created!\n");
        }
        else
        {
                printf("Sender thread created\n");
        }

	pthread_join(receiver,NULL);
	pthread_join(sender,NULL);

	mq_close(mymq);


	return 0;
}
