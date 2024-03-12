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



int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3)
    {
        if (rank == 0)
        {
            printf("Usage: %s <matrix_rows> <matrix_cols/vector_size>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int matrixRows = atoi(argv[1]);
    int matrixCols = atoi(argv[2]);

    if (matrixRows <= 0 || matrixCols <= 0)
    {
        if (rank == 0)
        {
            printf("Error: Matrix rows and columns must be greater than 0.\n");
        }
        MPI_Finalize();
        return 1;
    }

    srand(time(NULL) + rank); // Seed the random number generator uniquely for each process

    int rows_per_process = matrixRows / size;
    int remainder = matrixRows % size;
    int local_rows = rows_per_process + (rank < remainder ? 1 : 0);

    double *matrix = NULL;
    double *vector = createVector(matrixCols);
    double *result = NULL;
    double *local_matrix = (double *)malloc(local_rows * matrixCols * sizeof(double));
    double *local_result = (double *)malloc(local_rows * sizeof(double));

    if (rank == 0)
    {
        matrix = createMatrix(matrixRows, matrixCols);
        result = (double *)malloc(matrixRows * sizeof(double));
    }

    // Calculate the displacements for scattering and gathering
    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    for (int i = 0; i < size; i++)
    {
        sendcounts[i] = (rows_per_process + (i < remainder ? 1 : 0)) * matrixCols;
        displs[i] = (i > 0 ? displs[i - 1] + sendcounts[i - 1] : 0);
    }

    // Distribute the matrix to all processes
    MPI_Scatterv(matrix, sendcounts, displs, MPI_DOUBLE, local_matrix, local_rows * matrixCols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    // Distribute the matrix to all processes
    MPI_Scatterv(matrix, sendcounts, displs, MPI_DOUBLE, local_matrix, local_rows * matrixCols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    printf("Process %d received local matrix:\n", rank);
    for (int i = 0; i < local_rows; i++)
    {
        for (int j = 0; j < matrixCols; j++)
        {
            printf("%f ", local_matrix[i * matrixCols + j]);
        }
        printf("\n");
    }
    printf("\n");

    // Broadcast the vector to all processes
    MPI_Bcast(vector, matrixCols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Perform the local multiplication
    matrixVectorMultiply(local_matrix, vector, local_result, local_rows, matrixCols);
    // Perform the local multiplication
    matrixVectorMultiply(local_matrix, vector, local_result, local_rows, matrixCols);
    printf("Process %d local result:\n", rank);
    for (int i = 0; i < local_rows; i++)
    {
        printf("%f ", local_result[i]);
    }
    printf("\n");

    // Gather the local results into the final result vector
    MPI_Gatherv(local_result, local_rows, MPI_DOUBLE, result, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    // Gather the local results into the final result vector
    MPI_Gatherv(local_result, local_rows, MPI_DOUBLE, result, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        printf("Final result vector:\n");
        for (int i = 0; i < matrixRows; i++)
        {
            printf("%f\n", result[i]);
        }
    }

    if (rank == 0)
    {
        matrix = createMatrix(matrixRows, matrixCols);
        printf("Generated matrix:\n");
        for (int i = 0; i < matrixRows; i++)
        {
            for (int j = 0; j < matrixCols; j++)
            {
                printf("%f ", matrix[i * matrixCols + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    // Cleanup
    if (rank == 0)
    {
        free(matrix);
        free(result);
    }
    free(local_matrix);
    free(local_result);
    free(vector);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}



