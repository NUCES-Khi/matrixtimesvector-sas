// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <assert.h>
// #include <mpi.h>

// // Function to dynamically allocate a matrix and fill it with random values
// double *createMatrix(int rows, int cols, int seed)
// {
//     // srand(seed); // Seed the random number generator
//     double *matrix = (double *)malloc(rows * cols * sizeof(double));
//     if (matrix == NULL) {
//         // Handle memory allocation failure
//         fprintf(stderr, "Memory allocation failed for matrix.\n");
//         MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
//     }
//     for (int i = 0; i < rows * cols; i++)
//     {
//         matrix[i] = rand();
//     }
//     return matrix;
// }

// // Function to dynamically allocate a vector and fill it with random values
// double *createVector(int size, int seed)
// {
//     // srand(seed); // Seed the random number generator
//     double *vector = (double *)malloc(size * sizeof(double));
//     if (vector == NULL) {
//         // Handle memory allocation failure
//         fprintf(stderr, "Memory allocation failed for vector.\n");
//         MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
//     }
//     for (int i = 0; i < size; i++)
//     {
//         vector[i] = rand();
//     }
//     return vector;
// }

// // Function for tiled matrix-vector multiplication using MPI
// void matrixVectorMultiplyTiledMPI(double* local_tiles, double* vector, double* local_results, int num_local_rows, int matrix_cols, int tileSize, int rank, int size) {
//     // Initialize local results to zero
//     for (int i = 0; i < num_local_rows; i++) {
//         local_results[i] = 0.0;
//     }

//     // Perform the multiplication on local tiles
//     for (int i = 0; i < num_local_rows; i++) {
//         for (int j = 0; j < matrix_cols; j++) {
//             local_results[i] += local_tiles[i * matrix_cols + j] * vector[j];

//             char filename[256];
// sprintf(filename, "process%d.log", rank);
// FILE *file = fopen(filename, "a");
// if (file != NULL) {
//     fprintf(file, "Process %d, row %d, intermediate result: %f\n", rank, i, local_results[i]);
//     fclose(file);
// }

//         }
//     }
    
// }

// int main(int argc, char* argv[]) {
//     MPI_Init(&argc, &argv);
//     int rank, size;
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//     MPI_Comm_size(MPI_COMM_WORLD, &size);

//     // Ensure the correct number of arguments are provided
//     if (argc != 4) {
//         if (rank == 0) {
//             fprintf(stderr, "Usage: %s <matrix_rows> <matrix_cols> <tile_size>\n", argv[0]);
//         }
//         MPI_Abort(MPI_COMM_WORLD, 1);
//     }

//     int matrix_rows = atoi(argv[1]);
//     int matrix_cols = atoi(argv[2]);
//     int tile_size = atoi(argv[3]);
//     assert(matrix_rows % tile_size == 0 && matrix_cols % tile_size == 0); // Matrix dimensions must be divisible by tile size

//     // Calculate the number of rows per process
//     int rows_per_process = matrix_rows / size;
//     int remaining_rows = matrix_rows % size;
//     if (rank < remaining_rows) {
//         rows_per_process++;
//     }

//     // Allocate memory for local tiles and results
//     double* local_tiles = (double*)malloc(tile_size * matrix_cols * rows_per_process * sizeof(double));
//     double* local_results = (double*)calloc(rows_per_process, sizeof(double));
//     double* vector = (double*)malloc(matrix_cols * sizeof(double));

//     // Root process creates the full matrix and vector
//     double* matrix = NULL;
//     if (rank == 0) {
//         matrix = createMatrix(matrix_rows, matrix_cols, time(NULL));
//         vector = createVector(matrix_cols, time(NULL) + 1);
//     }

//     // Scatter the tiles of the matrix to all processes
//     MPI_Scatter(matrix, tile_size * matrix_cols * rows_per_process, MPI_DOUBLE, local_tiles, tile_size * matrix_cols * rows_per_process, MPI_DOUBLE, 0, MPI_COMM_WORLD);

//     // Broadcast the vector to all processes
//     MPI_Bcast(vector, matrix_cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

//     // Perform the local tiled multiplication
//     matrixVectorMultiplyTiledMPI(local_tiles, vector, local_results, rows_per_process, matrix_cols, tile_size, rank, size);

//     // Gather the local results into the final result vector
//     double* result = NULL;
//     if (rank == 0) {
//         result = (double*)malloc(matrix_rows * sizeof(double));
//     }
//     MPI_Gather(local_results, rows_per_process, MPI_DOUBLE, result, rows_per_process, MPI_DOUBLE, 0, MPI_COMM_WORLD);

//     // Root process prints the result
//     if (rank == 0) {
//         printf("Resulting vector:\n");
//         for (int i = 0; i < matrix_rows; i++) {
//             printf("%f\n", result[i]);
//         }
//         free(result);
//     }

//     // Cleanup
//     free(local_tiles);
//     free(local_results);
//     free(vector);
//     if (rank == 0) {
//         free(matrix);
//     }

//     MPI_Finalize();
//     return 0;
// }


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
void matrixVectorMultiplyTiledMPI(double* local_tiles, double* vector, double* local_results, int num_local_rows, int matrix_cols, int tileSize, int rank, int size) {
    // Initialize local results to zero
    for (int i = 0; i < num_local_rows; i++) {
        local_results[i] = 0.0;
    }

    // Perform the multiplication on local tiles
    for (int i = 0; i < num_local_rows; i++) {
        for (int j = 0; j < matrix_cols; j++) {
            local_results[i] += local_tiles[i * matrix_cols + j] * vector[j];
        }
    }
    
    // Write intermediate results to a log file
    char filename[256];
    sprintf(filename, "process%d.log", rank);
    FILE *file = fopen(filename, "a");
    if (file != NULL) {
        for (int i = 0; i < num_local_rows; i++) {
            fprintf(file, "Process %d, row %d, result: %f\n", rank, i, local_results[i]);
        }
        fclose(file);
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
            fprintf(stderr, "Usage: %s <matrix_rows> <matrix_cols> <tile_size>\n", argv[0]);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int matrix_rows = atoi(argv[1]);
    int matrix_cols = atoi(argv[2]);
    int tile_size = atoi(argv[3]);
    assert(matrix_rows % tile_size == 0 && matrix_cols % tile_size == 0); // Matrix dimensions must be divisible by tile size

    // Calculate the number of rows per process
    int rows_per_process = matrix_rows / size;
    int remaining_rows = matrix_rows % size;
    if (rank < remaining_rows) {
        rows_per_process++;
    }

    // Allocate memory for local tiles and results
    double* local_tiles = (double*)malloc(tile_size * matrix_cols * rows_per_process * sizeof(double));
    double* local_results = (double*)calloc(rows_per_process, sizeof(double));
    double* vector = (double*)malloc(matrix_cols * sizeof(double));

    // Root process creates the full matrix and vector
    double* matrix = NULL;
    if (rank == 0) {
        matrix = createMatrix(matrix_rows, matrix_cols, time(NULL));
        vector = createVector(matrix_cols, time(NULL) + 1);
    }

    // Scatter the tiles of the matrix to all processes
    MPI_Scatter(matrix, tile_size * matrix_cols * rows_per_process, MPI_DOUBLE, local_tiles, tile_size * matrix_cols * rows_per_process, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Broadcast the vector to all processes
    MPI_Bcast(vector, matrix_cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Perform the local tiled multiplication
    matrixVectorMultiplyTiledMPI(local_tiles, vector, local_results, rows_per_process, matrix_cols, tile_size, rank, size);

    // Gather the local results into the final result vector
    double* result = NULL;
    if (rank == 0) {
        result = (double*)malloc(matrix_rows * sizeof(double));
    }
    MPI_Gather(local_results, rows_per_process, MPI_DOUBLE, result, rows_per_process, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Root process prints the result
    if (rank == 0) {
        printf("Resulting vector:\n");
        for (int i = 0; i < matrix_rows; i++) {
            printf("%f\n", result[i]);
        }
        free(result);
    }

    // Cleanup
    free(local_tiles);
    free(local_results);
    free(vector);
    if (rank == 0) {
        free(matrix);
    }

    MPI_Finalize();
    return 0;
}
