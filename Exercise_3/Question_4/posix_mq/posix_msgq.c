/*@Author       : Om Raheja
 *@Date		: 3rd July 2019
 *@Reference	: https://users.cs.cf.ac.uk/Dave.Marshall/C/node30.html
 * 	     	: This code has been modified by me (originally written
 * 	       	  by Dr.Sam Siewert)as a part of the assignment for the
 * 	          Real Time embedded systems course at the University of 
 * 	          Colorado Boulder.
 *@Board used	: NVIDIA's JETSON NANO Development Board
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

/* Macros */
#define SNDRCV_MQ               "/send_receive_mq"
#define MAX_MSG_SIZE            128
#define RECEIVER_PRIORITY       99
#define SENDER_PRIORITY         98
#define ERROR                   (-1)

/* Global variables */
struct mq_attr msgqueue_attr;
static char canned_msg[] = "This is a test, and only a test, in the event of real emergency, you will be instructed....";

/* Sender Thread */
void *sender_thread()
{
        struct sched_param check_param;

        printf("Sender thread\n");
        mqd_t message_queue;
        int num_bytes;

        sched_getparam(0,&check_param);
        printf("Sender Thread Priority: %d\n",check_param.sched_priority);

	message_queue = mq_open(SNDRCV_MQ,O_RDWR,0664,&msgqueue_attr);

        if(message_queue < 0)
        {
                perror("Sender mq_open");
                exit(ERROR);
        }
        else
        {
                printf("Sender opened message queue!\n");
        }

        /* Send message with priority=30 */
        num_bytes = mq_send(message_queue,canned_msg,sizeof(canned_msg),30);

        if(num_bytes == ERROR)
        {
                perror("mq_send");
        }
        else
        {
                printf("Sender:Message sent successfully\n");
        }


}


/* Receiver Thread */
void *receiver_thread()
{
        struct sched_param check_param;

        printf("Receiver thread\n");
        mqd_t message_queue;
        char buffer[MAX_MSG_SIZE];
        int num_bytes;
        int msg_prio;

        sched_getparam(0,&check_param);
        printf("Receiver Thread Priority: %d\n",check_param.sched_priority);

        message_queue = mq_open(SNDRCV_MQ,(O_CREAT | O_RDONLY), 0664, &msgqueue_attr);
	if(message_queue < 0)
        {
                perror("Receiver mq_open");
                exit(ERROR);
        }
        else
        {
                printf("Receiver opened message queue!\n");
        }

        /* Read highest priority message from the queue */
        num_bytes = mq_receive(message_queue, buffer, MAX_MSG_SIZE, 0);

        if(num_bytes < 0)
        {
                perror("mq_receive");
        }
        else
        {
                buffer[num_bytes] = '\0';
                printf("Receiver:[Message:%s]\n",buffer);
        }
}


int main()
{
        /* thread id */
        pthread_t sender,receiver;

        /* instances for thread attributes */
        pthread_attr_t sender_attr,receiver_attr;

        /* instances for scheduling parameters */
        struct sched_param sender_sched_param, receiver_sched_param;


        /* Set message queue attributes */
        msgqueue_attr.mq_maxmsg = 100;
        msgqueue_attr.mq_msgsize = MAX_MSG_SIZE;
        msgqueue_attr.mq_flags = 0;

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

        /* Create threads */
        int rc = pthread_create(&receiver,&receiver_attr,receiver_thread,NULL);
        if(rc != 0)
        {
                printf("Receiver thread not created!\n");
        }
        else
        {
                printf("Receiver thread created\n");
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
	/* Wait for threads to finish their execution */
        pthread_join(receiver,NULL);
        pthread_join(sender,NULL);

        return 0;
}

