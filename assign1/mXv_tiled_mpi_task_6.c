#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <mpi.h>

// Function to dynamically allocate a matrix and fill it with random values
double *createMatrix(int rows, int cols, int seed)
{
    srand(seed); // Seed the random number generator
    double *matrix = (double *)malloc(rows * cols * sizeof(double));
    if (matrix == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed for matrix.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    for (int i = 0; i < rows * cols; i++)
    {
        matrix[i] = rand() / (double) RAND_MAX;
    }
    return matrix;
}

// Function to dynamically allocate a vector and fill it with random values
double *createVector(int size, int seed)
{
    srand(seed); // Seed the random number generator
    double *vector = (double *)malloc(size * sizeof(double));
    if (vector == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed for vector.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++)
    {
        vector[i] = rand() / (double) RAND_MAX;
    }
    return vector;
}

// Function for tiled matrix-vector multiplication using MPI
void matrixVectorMultiplyTiledMPI(double* localTiles, double* vector, double* localResults, int numLocalRows, int matrixCols, int tileSize, int rank, int size) {
    // Perform the multiplication on local tiles
    for (int i = 0; i < numLocalRows; i++) {
        for (int j = 0; j < matrixCols; j++) {
            localResults[i] += localTiles[i * matrixCols + j] * vector[j];
        }
    }
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure the correct number of arguments are provided
    if (argc != 4) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <matrixRows> <matrixCols> <tileSize>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int matrixRows = atoi(argv[1]);
    int matrixCols = atoi(argv[2]);
    int tileSize = atoi(argv[3]);

    // Check if matrix dimensions are divisible by tile size
    if (matrixRows % tileSize != 0 || matrixCols % tileSize != 0) {
        if (rank == 0) {
            fprintf(stderr, "Error: Matrix dimensions must be divisible by tileSize.\n");
        }
        MPI_Finalize();
        return 1;
    }

    // Calculate the number of rows per process
    int rowsPerProcess = matrixRows / size;
    int remainingRows = matrixRows % size;
    if (rank < remainingRows) {
        rowsPerProcess++;
    }

    int *sendCounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sendCounts[i] = (matrixRows / size) * matrixCols;
        if (i < remainingRows) {
            sendCounts[i] += matrixCols;
        }
        displs[i] = sum;
        sum += sendCounts[i];
    }

    // Allocate memory for local tiles and results
    double* localTiles = (double*)malloc(tileSize * matrixCols * rowsPerProcess * sizeof(double));
    double* localResults = (double*)calloc(rowsPerProcess, sizeof(double));
    double* vector = (double*)malloc(matrixCols * sizeof(double));

    // Root process creates the full matrix and vector
    double* matrix = NULL;
    if (rank == 0) {
        matrix = createMatrix(matrixRows, matrixCols, time(NULL));
        vector = createVector(matrixCols, time(NULL) + 1);
    }

    // Scatter the tiles of the matrix to all processes
    MPI_Scatter(matrix, tileSize * matrixCols * rowsPerProcess, MPI_DOUBLE, localTiles, tileSize * matrixCols * rowsPerProcess, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Broadcast the vector to all processes
    MPI_Bcast(vector, matrixCols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Perform the local tiled multiplication
    matrixVectorMultiplyTiledMPI(localTiles, vector, localResults, rowsPerProcess, matrixCols, tileSize, rank, size);

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
    free(localTiles);
    free(localResults);
    free(vector);
    if (rank == 0) {
        free(matrix);
    }

    MPI_Finalize();
    return 0;
}

