/****************************************************************
 *
 * ppm.h
 *
 * Read and write PPM files.  Only works for "raw" format.
 *
 ****************************************************************/

#ifndef PPM_H
#define PPM_H

#include <sys/types.h>

typedef struct Image
{
	  int width;
	  int height;
	  unsigned char *data;
} Image;

// Create an image of the specified width/height.
Image *ImageCreate(int width, int height);
	
// Read the image from the specified file.
Image *ImageRead(char const *filename);
// Write the image to the specified file.
void   ImageWrite(Image *image, char const *filename);

// Returns width/height of the image.
int    ImageWidth(Image *image);
int    ImageHeight(Image *image);

// Overwrite the entire image with the specified color 
void   ImageClear(Image *image, unsigned char red, unsigned char green, unsigned char blue);

// Set the color channel (chan) of the pixel at position (x,y) to val.
void   ImageSetPixel(Image *image, int x, int y, int chan, unsigned char val);
	
// Returns the value of the color channel (chan) of the pixel at position (x,y).
unsigned char ImageGetPixel(Image *image, int x, int y, int chan);

#endif 
