#include <cstdio>
#include <cstdlib>
#include <cmath>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <ctime>
#include <dispenso/parallel_for.h>

#define MAX_ITR 100
#define IMG_W 1920
#define IMG_H 1080
#define CHANNELS 3

struct C
{
    double real;
    double imag;
};

void complexAdd(C *z, C *cnst, C *res)
{
    res->real = z->real + cnst->real;
    res->imag = z->imag + cnst->imag;
}

void complexMultiply(C *x, C *y, C *res)
{
    res->real = (x->real * y->real) - (x->imag * y->imag);
    res->imag = (x->real * y->imag) + (x->imag * y->real);
}

double complexAbsolute(C *c)
{
    return sqrt((c->real * c->real) + (c->imag * c->imag));
}

int mandelbrot(C *cnst)
{
    C z = {0.0, 0.0};
    C zSq;

    for (int i = 0; i < MAX_ITR; i++)
    {
        if (complexAbsolute(&z) > 2)
        {
            return i;
        }

        complexMultiply(&z, &z, &zSq);
        complexAdd(&zSq, cnst, &z);
    }

    return MAX_ITR;
}

void getColor(int itrs, unsigned char *r, unsigned char *g, unsigned char *b)
{
    float t = itrs / (float)MAX_ITR;
    *r = (unsigned char)(9 * (1 - t) * t * t * t * 255);
    *g = (unsigned char)(15 * (1 - t) * (1 - t) * t * t * 255);
    *b = (unsigned char)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
}

int main()
{
    double REAL_MIN = -2.0;
    double REAL_MAX = 1.0;
    double IMAG_MIN = -0.85;
    double IMAG_MAX = 0.8375;

    double INC_REAL = (REAL_MAX - REAL_MIN) / IMG_W;
    double INC_IMAG = (IMAG_MAX - IMAG_MIN) / IMG_H;

    size_t img_bytes = IMG_W * IMG_H * CHANNELS * sizeof(unsigned char);
    unsigned char *host_image = (unsigned char *)malloc(img_bytes);

    if (host_image == NULL)
    {
        fprintf(stderr, "FAILED TO ALLOCATE MEMORY\n");
        return 1;
    }

    clock_t start_time = clock();

    dispenso::parallel_for(
        dispenso::makeChunkedRange(0, IMG_H, dispenso::ParForChunking::kAuto),
        [&](int yBegin, int yEnd) {
            for (int y = yBegin; y < yEnd; y++)
            {
                for (int x = 0; x < IMG_W; x++)
                {
                    double real = REAL_MIN + x * INC_REAL;
                    double imag = IMAG_MIN + y * INC_IMAG;

                    C c = {real, imag};
                    int itrs = mandelbrot(&c);

                    int idx = (y * IMG_W + x) * CHANNELS;
                    unsigned char r, g, b;
                    getColor(itrs, &r, &g, &b);

                    host_image[idx + 0] = r;
                    host_image[idx + 1] = g;
                    host_image[idx + 2] = b;
                }
            }
        }
    );

    clock_t end_time = clock();

    double elapsed_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Function took %f seconds to execute.\n", elapsed_seconds);

    if (!stbi_write_png("mandelbrot_set_2.png", IMG_W, IMG_H, CHANNELS, host_image, IMG_W * CHANNELS))
    {
        fprintf(stderr, "Failed to write PNG\n");
        free(host_image);
        return 1;
    }

    free(host_image);
    return 0;
}
