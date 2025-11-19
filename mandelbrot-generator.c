#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <pthread.h>
#include <time.h>

#define MAX_ITR 100
#define NUM_THREADS 32 * 32
#define IMG_W 1920
#define IMG_H 1080
#define CHANNELS 3
#define NUM_BLOCKS (int)ceil(((double)IMG_W * IMG_H) / NUM_THREADS)

typedef struct complexNumber
{ // we can only use `double` on CUDA, not `long double` (why?)
    double real;
    double imag;
} C;

typedef struct
{
    int startY;
    int endY;
    unsigned char *image;
    double REAL_MIN, IMAG_MIN;
    double INC_REAL, INC_IMAG;
} ThreadJob;

void complexAdd(C *z, C *cnst, C *res) // resulting is real parts summed and complex parts summed
{
    res->real = z->real + cnst->real;
    res->imag = z->imag + cnst->imag;
}

void complexMultiply(C *x, C *y, C *res) //
{
    res->real = (x->real * y->real) - (x->imag * y->imag);
    res->imag = (x->real * y->imag) + (x->imag * y->real);
}

double complexAbsolute(C *c)
{
    return sqrt((c->real * c->real) + (c->imag * c->imag)); // `sqrtf` only for floats
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

void *threadFunc(void *arg)
{
    ThreadJob *job = (ThreadJob *)arg;

    for (int y = job->startY; y < job->endY; y++)
    {
        for (int x = 0; x < IMG_W; x++)
        {
            double real = job->REAL_MIN + x * job->INC_REAL;
            double imag = job->IMAG_MIN + y * job->INC_IMAG;

            C c = {real, imag};
            int itrs = mandelbrot(&c);

            int idx = (y * IMG_W + x) * CHANNELS;
            unsigned char r, g, b;
            getColor(itrs, &r, &g, &b);

            job->image[idx + 0] = r;
            job->image[idx + 1] = g;
            job->image[idx + 2] = b;
        }
    }

    return NULL;
}

int main()
{
    int thread_count = 8;
    pthread_t threads[thread_count];
    ThreadJob jobs[thread_count];

    int rows_per_thread = IMG_H / thread_count;

    double REAL_MIN = -2.0;
    double REAL_MAX = 1.0;
    double IMAG_MIN = -0.85;
    double IMAG_MAX = 0.8375;

    double INC_REAL = (REAL_MAX - REAL_MIN) / IMG_W;
    double INC_IMAG = (IMAG_MAX - IMAG_MIN) / IMG_H;

    size_t img_bytes = IMG_W * IMG_H * CHANNELS * sizeof(unsigned char);
    unsigned char *host_image = (unsigned char *)malloc(img_bytes);

    clock_t start_time = clock();

    for (int i = 0; i < thread_count; i++)
    {
        jobs[i].startY = i * rows_per_thread;
        jobs[i].endY = (i == thread_count - 1 ? IMG_H : (i + 1) * rows_per_thread);
        jobs[i].image = host_image;
        jobs[i].REAL_MIN = REAL_MIN;
        jobs[i].INC_REAL = INC_REAL;
        jobs[i].IMAG_MIN = IMAG_MIN;
        jobs[i].INC_IMAG = INC_IMAG;

        pthread_create(&threads[i], NULL, threadFunc, &jobs[i]);
    }

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_t end_time = clock();

    double elapsed_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Function took %f seconds to execute.\n", elapsed_seconds);

    if (host_image == NULL)
    {
        fprintf(stderr, "FAILED TO ALLOCATE MEMORY\n");
        return 1;
    }

    if (!stbi_write_png("mandelbrot_set.png", IMG_W, IMG_H, CHANNELS, host_image, IMG_W * CHANNELS)) // this is why you can allocate W * H * CHANNELS
    {
        fprintf(stderr, "Failed to write PNG\n");
        return 1;
    }

    free(host_image);
}