 # Fast Box Blur (for PPM format) in C

 This implementation uses an approach where the sums of all the pixels in the
 rectangle defined, for each pixel, from (0, 0) to the pixel is pre-computed.
 This sum is then used to quickly compute the average pixel value for each
 pixel in the image.

 OpemMP is used to implement threading. A chunk size of 4 was determined
 experimentally to be the optimal size for work distribution on an Intel i7
 quad-core (8 logical cores). This number may differ from system to system.

## Performance
On an Intel i7 quad-core (8 logical core) machine, this algorithm blurs an
4928x3280 image in about 0.3748s (25 samples).
