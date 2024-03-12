#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

// Function to dynamically allocate a matrix and fill it with random values
double *createMatrix(int rows, int cols)
{
    double *matrix = (double *)malloc(rows * cols * sizeof(double));
    for (int i = 0; i < rows * cols; i++)
    {
        matrix[i] = rand() / (double)RAND_MAX;
    }
    return matrix;
}
