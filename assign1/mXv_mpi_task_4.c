/*
 * Programmer(s) : Syed Saad Ullah Hussaini, K214703 Ali Raza, K213100 Muhammad Sameed
 * Date: 12 March 2024
 * Desc: MPI Naive version of matrix vector multiplication. 
 */

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

void matrixVectorMultiply(double *matrix, double *vector, double *result, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        result[i] = 0.0;
        for (int j = 0; j < cols; j++)
        {
            result[i] += matrix[i * cols + j] * vector[j];
        }
    }
}



int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure the correct number of arguments are provided
    if (argc != 3) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <matrixRows> <matrixCols>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int matrixRows = atoi(argv[1]);
    int matrixCols = atoi(argv[2]);

    // The number of columns in the matrix must equal the size of the vector
    if (matrixRows <= 0 || matrixCols <= 0) {
        printf("Error: Matrix rows and columns must be greater than 0.\n");
        return 1;
    }

    // Calculate the number of rows per process
    int rowsPerProcess = matrixRows / size;
    int remainingRows = matrixRows % size;
    if (rank < remainingRows) {
        rowsPerProcess++;
    }

    // Allocate memory for local matrix and results
    double* localMatrix = (double*)malloc(matrixCols * rowsPerProcess * sizeof(double));
    double* localResults = (double*)calloc(rowsPerProcess, sizeof(double));
    double* vector = NULL;

    // Root process creates the full matrix and vector
    double* matrix = NULL;
    if (rank == 0) {
        matrix = createMatrix(matrixRows, matrixCols);
        vector = createVector(matrixCols);
    } else {
        vector = (double*)malloc(matrixCols * sizeof(double));
    }

    // Scatter the matrix to all processes
    MPI_Scatter(matrix, matrixCols * rowsPerProcess, MPI_DOUBLE, localMatrix, matrixCols * rowsPerProcess, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Broadcast the vector to all processes
    MPI_Bcast(vector, matrixCols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Perform the local matrix-vector multiplication
    matrixVectorMultiply(localMatrix, vector, localResults, rowsPerProcess, matrixCols);

    // Gather the local results into the final result vector
    double* result = NULL;
    if (rank == 0) {
        result = (double*)malloc(matrixRows * sizeof(double));
    }
    MPI_Gather(localResults, rowsPerProcess, MPI_DOUBLE, result, rowsPerProcess, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Root process prints the result
    if (rank == 0) {
        printf("Resulting vector:\n");
        for (int i = 0; i < matrixRows; i++) {
            printf("%f\n", result[i]);
        }
        free(result);
    }

    // Cleanup
    free(localMatrix);
    free(localResults);
    free(vector);
    if (rank == 0) {
        free(matrix);
    }

    MPI_Finalize();
    return 0;
}
