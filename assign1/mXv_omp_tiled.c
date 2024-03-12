#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//-- Function Declarations --//
int mv_omp(int n, double a[], double b[], double c[]);
void mv_gen(int n, double a[], double b[]);
void print_v(int n, double a[]);
//--Functions Declared--//

#ifndef TILE_DIM
#define TILE_DIM 64
#endif

int NUM_THREADS = 24;
int main(int argc, char* argv[]) {
    double *a, *b, *c, secs;
    int n;
    //--Command Line Input--//
    if (argc < 3) { return -1; }
    n = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    //--Allocate Memory--//
    a = (double*)malloc((n * n) * sizeof(double));
    b = (double*)malloc((n) * sizeof(double));
    c = (double*)malloc((n) * sizeof(double));
    mv_gen(n, a, b);
    //--Timing Version --//
    secs = clock();
    mv_omp(n, a, b, c);
    secs = clock() - secs;
    printf("OMP Version took %f seconds\n", secs / CLOCKS_PER_SEC);

    //print_v(n, c);

    return 0;
}// end main

int mv_omp(int n, double a[], double b[], double c[]) {
    int ii, jj, i, j;
    double ci;
#pragma omp parallel for collapse(2) \
private(ii,jj,i,j,ci)
    for (ii = 0; ii < n; ii += TILE_DIM)
        for (jj = 0; jj < n; jj += TILE_DIM) {
            for (i = ii; i < ii + TILE_DIM; ++i) {
                ci = 0.0;
                for (j = jj; j < jj + TILE_DIM; ++j)
                    ci += a[i*n + j] * b[j];
                c[i] += ci;
            }//end for i
        }//end for jj
    return 1;
}//end function

/// @brief Generate random values for matrix and vector
/// @param n dimension of the matrix and vector
/// @param a matrix: memory must be allocated via call to malloc or calloc
/// @param b vector: memory must be allocated via call to malloc or calloc
void mv_gen(int n, double a[], double b[]) {
    int i, j, seed = 1325;
    //--Set the matrix A and vector B.--//
    for (j = 0; j < n; j++) {
        for (i = 0; i < n; i++) {
            seed = (3125 * seed) % 65536;
            a[i + j * n] = (seed - 32768.0) / 16384.0;
        }
        seed = (3125 * seed) % 65536;
        b[j] = (seed - 32768.0) / 16384.0;
    }
    return;
}

void print_v(int n, double a[]) {
    for (int i = 0; i < n; i++) {
        printf("%f\n", a[i]);
    }
}
