#ifndef PPM_DUMP_H
#define PPM_DUMP_H

/*@Author       : Om Raheja
 *@File Name    : ppm_dump.h
 *@Date         :
 *@Board used   : NVIDIA's Jetson Nano running Linux.
 *@Tools        : Compiler:gcc ; Editor: Vim ; Camera: LogiTech C615
 *@Brief	: Dumps the obtained data into ppm format files.
 */

/* Standard C library headers */
#include <time.h>

/*@Function Name: dump_ppm
 *@Brief        : Writes the pixel values into an image.
 *@Param in [1] : const  void *p (Buffer which has to be converted to an image)
 *@Param in [2] : int size (Size of the Buffer being passed)
 *@Param in [3] : unsigned int tag (Tag for Header)
 *@Param in [4] : struct timespec *time (Used to calculate timestamp for the Image header)
 *@Return       : void
 * */
void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);


/*@Function Name: dump_ppm
 *@Brief        : Converts the obtained pixel data into ppm format files.
 *@Param in [1] : threadp
 *@Return       : void
 * */
void *Service_2(void *threadp);


#endif
