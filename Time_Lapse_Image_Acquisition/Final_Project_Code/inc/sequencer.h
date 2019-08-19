#ifndef SEQUENCER_H
#define SEQUENCER_H
 
/*@Author       : Om Raheja
 *@File Name    : sequencer.h 
 *@Date         : 8/11/2019
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Contains function prototypes.
 */

/* Function prototypes */

/*@Function Name: Sequencer
 *@Brief        : Runs at one hertz and schedules the other service depending upon the
 		: request frequency by posting semaphores.
 *@Param in     : void *threadp
 *@Return       : void
 * */
void *Sequencer(void *threadp);

#endif
