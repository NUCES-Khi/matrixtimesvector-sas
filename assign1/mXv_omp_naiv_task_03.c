/*
 * Programmer(s) : Syed Saad Ullah Hussaini, K214703 Ali Raza, K213100 Muhammad Sameed
 * Date: 12 March 2024
 * Desc: OMP Naive version of matrix vector multiplication. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Function to dynamically allocate a matrix and fill it with random values
double** createMatrix(int rows, int cols) {
    double** matrix = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)malloc(cols * sizeof(double));
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() / (double)RAND_MAX;
        }
    }
    return matrix;
}

// Function to dynamically allocate a vector and fill it with random values
double* createVector(int size) {
    double* vector = (double*)malloc(size * sizeof(double));
    for (int i = 0; i < size; i++) {
        vector[i] = rand() / (double)RAND_MAX;
    }
    return vector;
}

// Function for matrix-vector multiplication using OpenMP
void matrixVectorMultiplyOpenMP(double** matrix, double* vector, double* result, int rows, int cols) {
    #pragma omp parallel for
    for (int i = 0; i < rows; i++) {
        result[i] = 0.0;
        for (int j = 0; j < cols; j++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <matrix_rows> <matrix_cols/vector_size>\n", argv[0]);
        return 1;
    }

    int matrixRows = atoi(argv[1]);
    int matrixCols = atoi(argv[2]);

    // The number of columns in the matrix must equal the size of the vector
    if (matrixRows <= 0 || matrixCols <= 0) {
        printf("Error: Matrix rows and columns must be greater than 0.\n");
        return 1;
    }

    // Seed the random number generator
    srand(time(NULL));

    // Create and fill the matrix and vector with random values
    double** matrix = createMatrix(matrixRows, matrixCols);
    double* vector = createVector(matrixCols); // The vector size is the same as the number of columns in the matrix
    double* result = (double*)malloc(matrixRows * sizeof(double)); // The result vector size is the same as the number of rows in the matrix

    // Perform the matrix-vector multiplication through Naive OpemMP
    matrixVectorMultiplyOpenMP(matrix, vector, result, matrixRows, matrixCols);

    // Print the generated matrix
    printf("Generated matrix:\n");
    for (int i = 0; i < matrixRows; i++) {
        for (int j = 0; j < matrixCols; j++) {
            printf("%f ", matrix[i][j]);
        }
        printf("\n");
    }

    printf("\n");

    // Print the generated vector
    printf("Generated vector:\n");
    for (int i = 0; i < matrixCols; i++) {
        printf("%f ", vector[i]);
    }
    printf("\n\n");

    // Print the resultant vector
    printf("Resulting vector:\n");
    for (int i = 0; i < matrixRows; i++) {
        printf("%f\n", result[i]);
    }

    // Cleanup
    for (int i = 0; i < matrixRows; i++) {
        free(matrix[i]);
    }
    free(matrix);
    free(vector);
    free(result);

    return 0;
}

