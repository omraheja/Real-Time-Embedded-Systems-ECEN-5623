/*@Author 	: Dr.Sam Siewert
 *@Modified by	: Om Raheja
 *@Reference	: This code has been written by Professor Sam Siewert.
 *@brief 	:This code was modified in order to test feasibility of
 *	      	different task sets. The task sets were subjected to 
 *	      	completion time test and scheduling point test.
 *	      	If the task set passes both the tests then it is said
 *	      	to be schedulable.
 *@date		: 21st June 2019
 *@Board used	: NVIDIA's JETSON NANO Development Board
 * */


/*Standard C library headers */
#include <math.h>
#include <stdio.h>

/* Macros */
#define TRUE 1
#define FALSE 0
#define U32_T unsigned int


/* Task set declaration and initialization  */

// U=0.7333
U32_T ex0_period[] = {2, 10, 15};
U32_T ex0_wcet[] = {1, 1, 2};

// U=0.9857
U32_T ex1_period[] = {2, 5, 7};
U32_T ex1_wcet[] = {1, 1, 2};

// U=0.9967
U32_T ex2_period[] = {2, 5, 7, 13};
U32_T ex2_wcet[] = {1, 1, 1, 2};

// U=0.93
U32_T ex3_period[] = {3, 5, 15};
U32_T ex3_wcet[] = {1, 2, 3};

// U=1.0
U32_T ex4_period[] = {2, 4, 16};
U32_T ex4_wcet[] = {1, 1, 4};

// U=1.0
U32_T ex5_period[] = {2, 5, 10};
U32_T ex5_wcet[] = {1, 2, 1};

// U=1.0
U32_T ex6_period[] = {3, 6, 9};
U32_T ex6_wcet[] = {1, 2, 3};

// U=1.0
U32_T ex7_period[] = {2,4,7};
U32_T ex7_wcet[] = {1,1,1};

// U=1.0
U32_T ex8_period[] = {2,5,10};
U32_T ex8_wcet[] = {1,1,3};



/* Function Prototype */
int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int scheduling_point_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);


int main(void)
{ 
    int i;
	U32_T numServices;
    
    printf("******** Completion Test Feasibility Example\n");
   
    printf("Ex-0 U=%4.2f (C1=1, C2=1, C3=2; T1=2, T2=10, T3=15; T=D): ",
		   ((1.0/2.0) + (1.0/10.0) + (2.0/15.0)));
	numServices = sizeof(ex0_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex0_period, ex0_wcet, ex0_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

    printf("Ex-1 U=%4.2f (C1=1, C2=1, C3=2; T1=2, T2=5, T3=7; T=D): ", 
		   ((1.0/2.0) + (1.0/5.0) + (1.0/7.0)));
	numServices = sizeof(ex1_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex1_period, ex1_wcet, ex1_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");
	
    printf("Ex-2 U=%4.2f (C1=1, C2=1, C3=1, C4=2; T1=2, T2=5, T3=7, T4=13; T=D): ",
		   ((1.0/2.0) + (1.0/5.0) + (1.0/7.0) + (2.0/13.0)));
	numServices = sizeof(ex2_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex2_period, ex2_wcet, ex2_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

    printf("Ex-3 U=%4.2f (C1=1, C2=2, C3=3; T1=3, T2=5, T3=15; T=D): ",
		   ((1.0/3.0) + (2.0/5.0) + (3.0/15.0)));
	numServices = sizeof(ex3_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex3_period, ex3_wcet, ex3_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");
	
    printf("Ex-4 U=%4.2f (C1=1, C2=1, C3=4; T1=2, T2=4, T3=16; T=D): ",
		   ((1.0/2.0) + (1.0/4.0) + (4.0/16.0)));
	numServices = sizeof(ex4_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex4_period, ex4_wcet, ex4_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


    printf("Ex-5 U=%4.2f (C1=1, C2=2, C3=1; T1=2, T2=5, T3=10; T=D): ",
                   ((1.0/2.0) + (2.0/5.0) + (1.0/10.0)));
        numServices = sizeof(ex5_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex5_period, ex5_wcet, ex5_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


	 printf("Ex-6 U=%4.2f (C1=1, C2=2, C3=3; T1=3, T2=6, T3=9; T=D): ",
                   ((1.0/3.0) + (2.0/6.0) + (3.0/9.0)));
        numServices = sizeof(ex6_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex6_period, ex6_wcet, ex6_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


    printf("Ex-7 U=%4.2f (C1=1, C2=1, C3=1; T1=2, T2=4, T3=7; T=D): ",
                   ((1.0/2.0) + (1.0/4.0) + (1.0/7.0)));
        numServices = sizeof(ex7_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex7_period, ex7_wcet, ex7_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


 printf("Ex-8 U=%4.2f (C1=1, C2=1, C3=3; T1=2, T2=5, T3=10; T=D): ",
                   ((1.0/2.0) + (1.0/5.0) + (3.0/10.0)));
        numServices = sizeof(ex8_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex8_period, ex8_wcet, ex8_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");



	printf("\n\n");
    printf("******** Scheduling Point Feasibility Example\n");

    printf("Ex-0 U=%4.2f (C1=1, C2=1, C3=2; T1=2, T2=10, T3=15; T=D): ",
		   ((1.0/2.0) + (1.0/10.0) + (2.0/15.0)));
	numServices = sizeof(ex0_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex0_period, ex0_wcet, ex0_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

    printf("Ex-1 U=%4.2f (C1=1, C2=1, C3=2; T1=2, T2=5, T3=7; T=D): ", 
		   ((1.0/2.0) + (1.0/5.0) + (1.0/7.0)));
	numServices = sizeof(ex1_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex1_period, ex1_wcet, ex1_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");
	
    printf("Ex-2 U=%4.2f (C1=1, C2=1, C3=1, C4=2; T1=2, T2=5, T3=7, T4=13; T=D): ",
		   ((1.0/2.0) + (1.0/5.0) + (1.0/7.0) + (2.0/13.0)));
	numServices = sizeof(ex2_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex2_period, ex2_wcet, ex2_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

    printf("Ex-3 U=%4.2f (C1=1, C2=2, C3=3; T1=3, T2=5, T3=15; T=D): ",
		   ((1.0/3.0) + (2.0/5.0) + (3.0/15.0)));
	numServices = sizeof(ex3_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex3_period, ex3_wcet, ex3_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");
	
    printf("Ex-4 U=%4.2f (C1=1, C2=1, C3=4; T1=2, T2=4, T3=16; T=D): ",
		   ((1.0/2.0) + (1.0/4.0) + (4.0/16.0)));
	numServices = sizeof(ex4_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex4_period, ex4_wcet, ex4_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");



     printf("Ex-5 U=%4.2f (C1=1, C2=2, C3=1; T1=2, T2=5, T3=10; T=D): ",
                   ((1.0/2.0) + (2.0/5.0) + (1.0/10.0)));
        numServices = sizeof(ex5_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex5_period, ex5_wcet, ex5_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


	printf("Ex-6 U=%4.2f (C1=1, C2=2, C3=3; T1=3, T2=6, T3=9; T=D): ",
                   ((1.0/3.0) + (2.0/6.0) + (3.0/9.0)));
        numServices = sizeof(ex6_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex6_period, ex6_wcet, ex6_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


     printf("Ex-7 U=%4.2f (C1=1, C2=1, C3=1; T1=2, T2=4, T3=7; T=D): ",
                   ((1.0/2.0) + (1.0/4.0) + (1.0/7.0)));
        numServices = sizeof(ex7_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex7_period, ex7_wcet, ex7_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


    printf("Ex-8 U=%4.2f (C1=1, C2=1, C3=3; T1=2, T2=5, T3=10; T=D): ",
                   ((1.0/2.0) + (1.0/5.0) + (3.0/10.0)));
        numServices = sizeof(ex8_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex8_period, ex8_wcet, ex8_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");


}


/* @Funtion Name: Completion Time feasibility function
 * @Param in [1]: number of services
 * @Param in [2]: time period of tasks
 * @Param in [3]: wcet (worst case execution time) of tasks
 * @Param in [4]: Deadlines of taks
 *
 * @Param out  	: TRUE if task set is feasible
 * 		: FALSE if task set is infeasible
 * */
int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  int i, j;
  U32_T an, anext;
  
  // assume feasible until we find otherwise
  int set_feasible=TRUE;
   
  //printf("numServices=%d\n", numServices);
  
  for (i=0; i < numServices; i++)
  {
       an=0; anext=0;
       
       for (j=0; j <= i; j++)
       {
           an+=wcet[j];
       }
       
	   //printf("i=%d, an=%d\n", i, an);

       while(1)
       {
             anext=wcet[i];
	     
             for (j=0; j < i; j++)
                 anext += ceil(((double)an)/((double)period[j]))*wcet[j];
		 
             if (anext == an)
                break;
             else
                an=anext;

			 //printf("an=%d, anext=%d\n", an, anext);
       }
       
	   //printf("an=%d, deadline[%d]=%d\n", an, i, deadline[i]);

       if (an > deadline[i])
       {
          set_feasible=FALSE;
       }
  }
  
  return set_feasible;
}




/* @Funtion Name: Scheduling time feasibility function
 * @Param in [1]: number of services
 * @Param in [2]: time period of tasks
 * @Param in [3]: wcet (worst case execution time) of tasks
 * @Param in [4]: Deadlines of taks
 *
 * @Param out   : TRUE if task set is feasible
 *              : FALSE if task set is infeasible
 * */

int scheduling_point_feasibility(U32_T numServices, U32_T period[], 
								 U32_T wcet[], U32_T deadline[])
{
   int rc = TRUE, i, j, k, l, status, temp;

   for (i=0; i < numServices; i++) // iterate from highest to lowest priority
   {
      status=0;

      for (k=0; k<=i; k++) 
      {
          for (l=1; l <= (floor((double)period[i]/(double)period[k])); l++)
          {
               temp=0;

               for (j=0; j<=i; j++) temp += wcet[j] * ceil((double)l*(double)period[k]/(double)period[j]);

               if (temp <= (l*period[k]))
			   {
				   status=1;
				   break;
			   }
           }
           if (status) break;
      }
      if (!status) rc=FALSE;
   }
   return rc;
}
