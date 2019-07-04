/*@Author	: Om Raheja
 *@Date		: 25th June 2019 
 *@Filename	: thread.c
 *@Course	: Real Time Embedded Systems
 *@Board Used	: NVIDIA's Jetson Nano
 *@Tools used	: Gcc compiler; Vim Editor; MobaXTerm
 * */

/* Standard C Library Headers */
#include<stdio.h>
#include<pthread.h>
#include<time.h>
#include<stdlib.h>
#include<unistd.h>


/* Global variables */

typedef struct {
	double x_axis,y_axis,z_axis;
	double roll,pitch,yaw;
	struct timespec time;
}data_t;

pthread_mutex_t lock;
data_t data;
double timestamp;


/*@function name: fun1
 *@brief	: Updates the values of X,Y and Z axis.
 *		  Updates the values of Roll, Yaw and Pitch.
 *		  Updates Time stamp.	
 *@Param [in]	: void
 *@return	: void
 */

void *fun1()
{
		pthread_mutex_lock(&lock);
		printf("Updating values.........\n");
		data.x_axis = ((double)rand());
		data.y_axis = ((double)rand());
		data.z_axis = ((double)rand());
		data.roll = ((double)rand());
		data.pitch = ((double)rand());
		data.yaw = ((double)rand());
		clock_gettime(CLOCK_REALTIME,&(data.time));
		timestamp = (double)data.time.tv_sec;
		timestamp += (double)data.time.tv_nsec/(double)1000000000;
		pthread_mutex_unlock(&lock);
		printf("Updating TimeStamp = %lf\n",timestamp);
		printf("Updating X     = %lf\tUpdating Y     = %lf\tUpdating Z      = %lf\n",data.x_axis,data.y_axis,data.z_axis);
        	printf("Updating Yaw   = %lf\tUpdating Roll  = %lf\tUpdating Pitch  = %lf\t",data.yaw,data.roll,data.pitch);
}


/*@function name: fun2
 *@brief        : Reads the updated values of X,Y and Z axis.
 *                Reads the updated values of Roll, Yaw and Pitch.    
 *                Reads the updated time stamp.
 *@Param [in]   : void
 *@return       : void
 */
void *fun2()
{
		pthread_mutex_lock(&lock);
		printf("\nReading values..........\n");
		printf("Timestamp:[%lf secs]\n",timestamp);
		printf("X-axis = [%lf]\tY-axis = [%lf]\tZ-axis = [%lf]\n",data.x_axis,data.y_axis,data.z_axis);
		printf("Yaw    = [%lf]\tRoll   = [%lf]\tPitch  = [%lf]\n\n",data.yaw,data.roll,data.pitch);
		pthread_mutex_unlock(&lock);
}




int main()
{
	pthread_t thread1,thread2;
	pthread_mutex_init(&lock,NULL);

	srand(time(NULL));

	for(int i=0;i<10;i++)
	{
		printf("**************************************************** %d ITERATION ****************************************************\n",i);
		pthread_create(&thread1,NULL,fun1,NULL);
		usleep(1000);
		pthread_create(&thread2,NULL,fun2,NULL);
		pthread_join(thread1,NULL);
		pthread_join(thread2,NULL);
		printf("*********************************************************************************************************************\n");
	}
	
	pthread_mutex_destroy(&lock);
	return 0;
}
