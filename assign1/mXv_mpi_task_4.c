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
// Function to dynamically allocate a vector and fill it with random values
double *createVector(int size)
{
    double *vector = (double *)malloc(size * sizeof(double));
    for (int i = 0; i < size; i++)
    {
        vector[i] = rand() / (double)RAND_MAX;
    }
    return vector;
}



