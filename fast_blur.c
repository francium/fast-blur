/**
 * Fast Box Blur (PPM for images)
 *
 * This implementation uses an approach where the sums of all the pixels in the
 * rectangle defined, for each pixel, from (0, 0) to the pixel is pre-computed.
 * This sum is then used to quickly compute the average pixel value for each
 * pixel in the image.
 *
 * OpemMP is used to implement threading. A chunk size of 4 was determined
 * experimentally to be the optimal size for work distribution on an Intel i7
 * quad-core (8 logical cores). This number may differ from system to system.
 */

#include <stdlib.h>
#include <stdio.h>

#include "ppmFile.h"

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) < (Y)) ? (Y) : (X))

/**
 * Get linear index from a (row, col) for a linearly allocated 2D array.
 */
int idx(int row, int col, int width, int g) {
    return (row * width + col) * g;
}

/* Is provided for reference, but experiments show using a transpose leads to
 * slightly slower performance.
 */
// void transpose(unsigned char *matrix, int h, int w, int g) {
//     unsigned char *transposed_matrix
//         = malloc(g * h * w * sizeof(unsigned char));
//
//     // Slightly faster if transpose is parallel.
//     #pragma omp parallel for
//     for (int row = 0; row < h; row++) {
//         for (int col = 0; col < w; col++) {
//             for (int gi = 0; gi < g; gi++) {
//                 transposed_matrix[gi + idx(col, row, h, g)]
//                     = matrix[gi + idx(row, col, w, g)];
//             }
//         }
//     }
//
//     for (int i = 0; i < g * h * w; i++) {
//         matrix[i] = transposed_matrix[i];
//     }
//     free(transposed_matrix);
// }

int main(int argc, char *argv[]) {
    char *file_in_name = argv[2];
    char *file_out_name = argv[3];
    const int R = atoi(argv[1]);

    Image *img_in = ImageRead(file_in_name);
    const int H = img_in->height;
    const int W = img_in->width;

    Image *img_out = ImageCreate(W, H);

    // Sums of all rectangles, for each pixel, from (0, 0) to the pixel; one per
    // color channel.
    int *sums_r = malloc(sizeof(int) * H * W);
    int *sums_g = malloc(sizeof(int) * H * W);
    int *sums_b = malloc(sizeof(int) * H * W);

    // The work of computing the rectangular sums is divided into two parts to
    // enabled parallelization.

    // First part will compute, for each row, the sums of all pixels left of
    // each pixel.  The image pixel is accessed here to avoid performing an
    // additional pixel traversal in a separate double-for-loop structure to
    // initialize the sums_* matrices with image pixels.
    #pragma omp parallel for schedule(static, 4)
    for (int row = 0; row < H; row++) {
        for (int col = 1; col < W; col++) {
            sums_r[idx(row, col, W, 1)]
                = ImageGetPixel(img_in, col, row, 0) + sums_r[idx(row, col - 1, W, 1)];
            sums_g[idx(row, col, W, 1)]
                = ImageGetPixel(img_in, col, row, 1) + sums_g[idx(row, col - 1, W, 1)];
            sums_b[idx(row, col, W, 1)]
                = ImageGetPixel(img_in, col, row, 2) + sums_b[idx(row, col - 1, W, 1)];
        }
    }

    // The second part will compute, for each column, the sum of all pixels from
    // (0, 0) to the pixel by adding the sum of the pixel above (which contains
    // the sum of all pixels to its left) with the current pixel. This will
    // results in computation of the sums of all pixels from (0, 0) to the
    // current pixel.
    // We can transpose the image pixels to avoid accessing in column-major
    // direction, but experimentally, it was slower than performing this
    // computation in column-major. If transposing is done, the data must be
    // transposed again to get it back to the original layout after the
    // computation on the columns is done.
    #pragma omp parallel for schedule(static, 4)
    for (int col = 0; col < W; col++) {
        for (int row = 1; row < H; row++) {
            sums_r[idx(row, col, W, 1)] += sums_r[idx(row - 1, col, W, 1)];
            sums_g[idx(row, col, W, 1)] += sums_g[idx(row - 1, col, W, 1)];
            sums_b[idx(row, col, W, 1)] += sums_b[idx(row - 1, col, W, 1)];
        }
    }

    // Perform the blur value of each pixel
    #pragma omp parallel for schedule(static, 4)
    for (int row = 0; row < H; row++) {
        for (int col = 0; col < W; col++) {
            // Coordinated of the corners of the square surrounding the pixel.
            int x_min = max(col - R, 0);
            int x_max = min(col + R, W - 1);
            int y_min = max(row - R, 0);
            int y_max = min(row + R, H - 1);

            // Number of pixels in the square.
            int pixels = (x_max - (x_min - 1)) * (y_max - (y_min - 1));

            // Do for each color channel (red, green, blue).
            for (int color = 0; color < 3; color++) {
                int *sums_color
                    = color == 0 ? sums_r
                    : color == 1 ? sums_g
                    :              sums_b;

                // The computation occurring below can be visually described,
                //      0      m        n
                //    0 +------+--------+-> rows
                //      |  a   |   b    |
                //    p +------+--------+
                //      |      |        |
                //      |  c   |   d    |
                //      |      |        |
                //    q +------+--------+
                //      |
                //      v
                //     columns
                //
                //  Where,
                //     'a' is a rectangle from (0, 0) to (p, m)
                //     'b' is a rectangle from (0, 0) to (p, n)
                //     'c' is a rectangle from (0, 0) to (q, m)
                //     'd' is a rectangle from (0, 0) to (q, n)
                //
                // The current pixel is in the middle of the box from (p, m) to
                // (q, n). The sum of all the pixels in the box surrounding the
                // pixel is then equal to `d - (c + b - a)`.
                int a = y_min < 1 || x_min < 1
                    ? 0
                    : sums_color[idx(y_min - 1, x_min - 1, W, 1)];
                int b = y_min < 1
                    ? 0
                    : sums_color[idx(y_min - 1, x_max, W, 1)];
                int c = x_min < 1
                    ? 0
                    : sums_color[idx(y_max, x_min - 1, W, 1)];
                int d = sums_color[idx(y_max, x_max, W, 1)];

                // Pixel's blurred value
                unsigned char s = (float)(d - (b + c - a)) / pixels;

                ImageSetPixel(img_out, col, row, color, s);
            }

        }
    }

    ImageWrite(img_out, file_out_name);

    free(sums_r);
    free(sums_g);
    free(sums_b);

    return 0;
}
