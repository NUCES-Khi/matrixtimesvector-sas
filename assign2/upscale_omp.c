#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#pragma pack(push, 1)
typedef struct
{
    unsigned short type;
    unsigned int size;
    unsigned short reserved1, reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct
{
    unsigned int size;
    int width, height;
    unsigned short planes;
    unsigned short bitCount;
    unsigned int compression;
    unsigned int imageSize;
    int xPelsPerMeter, yPelsPerMeter;
    unsigned int clrUsed, clrImportant;
} BMPInfoHeader;
#pragma pack(pop)

unsigned char *loadBMP(const char *filename, BMPHeader *header, BMPInfoHeader *infoHeader)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return NULL;
    fread(header, sizeof(BMPHeader), 1, file);
    fread(infoHeader, sizeof(BMPInfoHeader), 1, file);
    if (header->type != 0x4D42)
    {
        fclose(file);
        return NULL;
    }
    unsigned char *data = (unsigned char *)malloc(infoHeader->imageSize);
    fseek(file, header->offset, SEEK_SET);
    fread(data, infoHeader->imageSize, 1, file);
    fclose(file);
    return data;
}

void saveBMP(const char *filename, BMPHeader *header, BMPInfoHeader *infoHeader, unsigned char *data)
{
    FILE *file = fopen(filename, "wb");
    fwrite(header, sizeof(BMPHeader), 1, file);
    fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);
    fseek(file, header->offset, SEEK_SET);
    fwrite(data, infoHeader->imageSize, 1, file);
    fclose(file);
}

// int clamp(double x, int minVal, int maxVal) {
//     return (int)(x < minVal ? minVal : (x > maxVal ? maxVal : x));
// }

int max(int a, int b) {
    return (a > b) ? a : b;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

double cubicInterpolate(double p[4], double x) {
    return p[1] + 0.5 * x * (p[2] - p[0] + x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] + x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
}

double interpolate(double arr[4], double x) {
    double vals[4] = {
        cubicInterpolate(arr, x),
        cubicInterpolate(arr + 1, x),
        cubicInterpolate(arr + 2, x),
        cubicInterpolate(arr + 3, x)
    };
    return cubicInterpolate(vals, x);
}

double cubicHermite(double A, double B, double C, double D, double t) {
    double a = -A/2.0 + (3.0*B)/2.0 - (3.0*C)/2.0 + D/2.0;
    double b = A - (5.0*B)/2.0 + 2.0*C - D / 2.0;
    double c = -A/2.0 + C/2.0;
    double d = B;

    return a*t*t*t + b*t*t + c*t + d;
}
int clamp(double x, int minVal, int maxVal) {
    return (int)(x < minVal ? minVal : (x > maxVal ? maxVal : x));
}

void bicubicInterpolate(const unsigned char* input, unsigned char* output, int width, int height, int channels, int newWidth, int newHeight) {
    double x_ratio = ((double)(width-1))/newWidth;
    double y_ratio = ((double)(height-1))/newHeight;
    double px, py; 
    for (int i = 0; i < newHeight; i++) {
        for (int j = 0; j < newWidth; j++) {
            px = x_ratio * j;
            py = y_ratio * i;
            int x = (int)px;
            int y = (int)py;
            double x_diff = px - x;
            double y_diff = py - y;

            for (int k = 0; k < channels; k++) {
                double intensities[4][4] = {0}; // Matrix to hold nearby intensities

                // Gather intensities from the 4x4 neighborhood
                for (int m = -1; m <= 2; m++) {
                    for (int n = -1; n <= 2; n++) {
                        int ix = min(max(x + n, 0), width - 1);
                        int iy = min(max(y + m, 0), height - 1);
                        intensities[m+1][n+1] = input[(iy * width + ix) * channels + k];
                    }
                }

                // Interpolate rows
                double col[4] = {0};
                for (int l = 0; l < 4; l++) {
                    col[l] = cubicHermite(intensities[l][0], intensities[l][1], intensities[l][2], intensities[l][3], x_diff);
                }

                // Interpolate column
                double value = cubicHermite(col[0], col[1], col[2], col[3], y_diff);
                output[(i * newWidth + j) * channels + k] = (unsigned char)clamp(value, 0, 255);
            }
        }
    }
}


void applyConvolution(const unsigned char *input, unsigned char *output, int width, int height, const int kernel[3][3], int kernelDiv)
{
#pragma omp parallel for collapse(2)
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sum = 0;
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int idx = (y + ky) * width + (x + kx);
                    sum += input[idx] * kernel[ky + 1][kx + 1];
                }
            }
            output[y * width + x] = (unsigned char)max(0, min(255, sum / kernelDiv));
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <input.bmp> <output.bmp> <num_threads>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[3]);

    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *inputData = loadBMP(argv[1], &header, &infoHeader);
    if (!inputData)
    {
        printf("Failed to load image\n");
        return 1;
    }

    int newWidth = infoHeader.width * 2; // Upscale factor of 2
    int newHeight = infoHeader.height * 2;
    unsigned char *tempData = (unsigned char *)calloc(newWidth * newHeight, infoHeader.bitCount / 8);
    unsigned char *outputData = (unsigned char *)calloc(newWidth * newHeight, infoHeader.bitCount / 8);

    if (tempData == NULL || outputData == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for image data\n");
        free(inputData);
        if (tempData != NULL)
            free(tempData);
        if (outputData != NULL)
            free(outputData);
        return 1;
    }

    bicubicInterpolate(inputData, tempData, infoHeader.width, infoHeader.height , 3, newWidth, newHeight);

    // Uncomment the following section if you want to apply edge detection
    
    int edgeKernel[3][3] = {
        {-1, -1, -1},
        {-1, 8, -1},
        {-1, -1, -1}};
    int kernelDiv = 1;
    applyConvolution(tempData, outputData, newWidth, newHeight, edgeKernel, kernelDiv);
    

    infoHeader.width = newWidth;
    infoHeader.height = newHeight;
    infoHeader.imageSize = newWidth * newHeight * (infoHeader.bitCount / 8);

    saveBMP(argv[2], &header, &infoHeader, outputData);

    free(inputData);
    free(tempData);
    free(outputData);
    return 0;
}


// #include <stdio.h>
// #include <stdlib.h>
// #include <omp.h>

// #pragma pack(push, 1)
// typedef struct {
//     unsigned short type;
//     unsigned int size;
//     unsigned short reserved1, reserved2;
//     unsigned int offset;
// } BMPHeader;

// typedef struct {
//     unsigned int size;
//     int width, height;
//     unsigned short planes;
//     unsigned short bitCount;
//     unsigned int compression;
//     unsigned int imageSize;
//     int xPelsPerMeter, yPelsPerMeter;
//     unsigned int clrUsed, clrImportant;
// } BMPInfoHeader;
// #pragma pack(pop)

// unsigned char* loadBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader) {
//     FILE* file = fopen(filename, "rb");
//     if (!file) return NULL;

//     fread(header, sizeof(BMPHeader), 1, file);
//     fread(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     if (header->type != 0x4D42) {
//         fclose(file);
//         return NULL;
//     }

//     unsigned char* data = (unsigned char*)malloc(infoHeader->imageSize);
//     fseek(file, header->offset, SEEK_SET);
//     fread(data, infoHeader->imageSize, 1, file);
//     fclose(file);

//     return data;
// }

// void saveBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader, unsigned char* data) {
//     FILE* file = fopen(filename, "wb");
//     fwrite(header, sizeof(BMPHeader), 1, file);
//     fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     fseek(file, header->offset, SEEK_SET);
//     fwrite(data, infoHeader->imageSize, 1, file);
//     fclose(file);
// }

// int main(int argc, char* argv[]) {
//     if (argc != 4) {
//         printf("Usage: %s <input.bmp> <output.bmp> <num_threads>\n", argv[0]);
//         return 1;
//     }

//     int num_threads = atoi(argv[3]);
//     omp_set_num_threads(num_threads);

//     BMPHeader header;
//     BMPInfoHeader infoHeader;
//     unsigned char* inputData = loadBMP(argv[1], &header, &infoHeader);
//     if (!inputData) {
//         printf("Failed to load image\n");
//         return 1;
//     }

//     int upscale_factor = 2; // Define your upscale factor
//     int newWidth = infoHeader.width * upscale_factor;
//     int newHeight = infoHeader.height * upscale_factor;
//     int newRowSize = (newWidth * infoHeader.bitCount + 31) / 32 * 4;
//     int newImageSize = newHeight * newRowSize;

//     unsigned char* outputData = (unsigned char*)malloc(newImageSize);

//     double start_time = omp_get_wtime();
//     #pragma omp parallel for collapse(2)
//     for (int y = 0; y < newHeight; y++) {
//         for (int x = 0; x < newWidth; x++) {
//             int oldX = x / upscale_factor;
//             int oldY = y / upscale_factor;
//             int oldIndex = (oldY * infoHeader.width + oldX) * 3;
//             int newIndex = (y * newWidth + x) * 3;
//             outputData[newIndex + 0] = inputData[oldIndex + 0];
//             outputData[newIndex + 1] = inputData[oldIndex + 1];
//             outputData[newIndex + 2] = inputData[oldIndex + 2];
//         }
//     }
//     double end_time = omp_get_wtime();

//     infoHeader.width = newWidth;
//     infoHeader.height = newHeight;
//     infoHeader.imageSize = newImageSize;

//     saveBMP(argv[2], &header, &infoHeader, outputData);

//     printf("Processing time with OpenMP: %f seconds\n", end_time - start_time);

//     free(inputData);
//     free(outputData);
//     return 0;
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
// #include <omp.h>

// #pragma pack(push, 1)
// typedef struct {
//     unsigned short type;
//     unsigned int size;
//     unsigned short reserved1, reserved2;
//     unsigned int offset;
// } BMPHeader;

// typedef struct {
//     unsigned int size;
//     int width, height;
//     unsigned short planes;
//     unsigned short bitCount;
//     unsigned int compression;
//     unsigned int imageSize;
//     int xPelsPerMeter, yPelsPerMeter;
//     unsigned int clrUsed, clrImportant;
// } BMPInfoHeader;
// #pragma pack(pop)

// unsigned char* loadBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader) {
//     FILE* file = fopen(filename, "rb");
//     if (!file) return NULL;

//     fread(header, sizeof(BMPHeader), 1, file);
//     fread(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     if (header->type != 0x4D42) {
//         fclose(file);
//         return NULL;
//     }

//     unsigned char* data = (unsigned char*)malloc(infoHeader->imageSize);
//     fseek(file, header->offset, SEEK_SET);
//     fread(data, infoHeader->imageSize, 1, file);
//     fclose(file);

//     return data;
// }

// void saveBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader, unsigned char* data) {
//     FILE* file = fopen(filename, "wb");
//     fwrite(header, sizeof(BMPHeader), 1, file);
//     fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     fseek(file, header->offset, SEEK_SET);
//     fwrite(data, infoHeader->imageSize, 1, file);
//     fclose(file);
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>

// double cubicInterpolate(double p[4], double x) {
//     return p[1] + 0.5 * x * (p[2] - p[0] + x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] + x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
// }

// double interpolate(double col[4], double x) {
//     double vals[4] = { cubicInterpolate(col, x) };
//     return cubicInterpolate(vals, x);
// }

// void bicubicInterpolate(const unsigned char* input, unsigned char* output, int width, int height, int newWidth, int newHeight) {
//     double x_ratio = ((double)(width - 1)) / newWidth;
//     double y_ratio = ((double)(height - 1)) / newHeight;
//     double px, py;
//     for (int i = 0; i < newHeight; i++) {
//         for (int j = 0; j < newWidth; j++) {
//             px = x_ratio * j;
//             py = y_ratio * i;
//             int fx = (int)floor(px);
//             int fy = (int)floor(py);
//             int cx = fx + 1;
//             int cy = fy + 1;
//             double dx = px - fx;
//             double dy = py - fy;

//             if (cx >= width)  cx = fx;
//             if (cy >= height) cy = fy;

//             double data[4][4];
//             for (int m = 0; m < 4; m++) {
//                 for (int n = 0; n < 4; n++) {
//                     int sx = fx + (n - 1);
//                     int sy = fy + (m - 1);
//                     if (sx < 0) sx = 0;
//                     if (sx >= width) sx = width - 1;
//                     if (sy < 0) sy = 0;
//                     if (sy >= height) sy = height - 1;
//                     data[m][n] = input[sy * width + sx];
//                 }
//             }

//             double col[4];
//             for (int k = 0; k < 4; k++) {
//                 col[k] = cubicInterpolate(data[k], dx);
//             }
//             double value = cubicInterpolate(col, dy);
//             output[i * newWidth + j] = (unsigned char)clamp(value, 0, 255);
//         }
//     }
// }

// int clamp(double x, int min, int max) {
//     if (x < min) return min;
//     if (x > max) return max;
//     return (int)x;
// }

// int main(int argc, char* argv[]) {
//     if (argc != 4) {
//         printf("Usage: %s <input.bmp> <output.bmp> <num_threads>\n", argv[0]);
//         return 1;
//     }

//     int num_threads = atoi(argv[3]);
//     omp_set_num_threads(num_threads);

//     BMPHeader header;
//     BMPInfoHeader infoHeader;
//     unsigned char* inputData = loadBMP(argv[1], &header, &infoHeader);
//     if (!inputData) {
//         printf("Failed to load image\n");
//         return 1;
//     }

//     int upscale_factor = 2; // Define your upscale factor
//     int newWidth = infoHeader.width * upscale_factor;
//     int newHeight = infoHeader.height * upscale_factor;
//     int newImageSize = newWidth * newHeight * (infoHeader.bitCount / 8);

//     unsigned char* outputData = (unsigned char*)malloc(newImageSize);

//     double start_time = omp_get_wtime();
//     bicubicInterpolate(inputData, outputData, infoHeader.width, infoHeader.height, newWidth, newHeight);
//     double end_time = omp_get_wtime();

//     infoHeader.width = newWidth;
//     infoHeader.height = newHeight;
//     infoHeader.imageSize = newImageSize;

//     saveBMP(argv[2], &header, &infoHeader, outputData);

//     printf("Processing time with OpenMP: %f seconds\n", end_time - start_time);

//     free(inputData);
//     free(outputData);
//     return 0;
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
// #include <omp.h>

// #pragma pack(push, 1)
// typedef struct
// {
//     unsigned short type;
//     unsigned int size;
//     unsigned short reserved1, reserved2;
//     unsigned int offset;
// } BMPHeader;

// typedef struct
// {
//     unsigned int size;
//     int width, height;
//     unsigned short planes;
//     unsigned short bitCount;
//     unsigned int compression;
//     unsigned int imageSize;
//     int xPelsPerMeter, yPelsPerMeter;
//     unsigned int clrUsed, clrImportant;
// } BMPInfoHeader;
// #pragma pack(pop)

// unsigned char *loadBMP(const char *filename, BMPHeader *header, BMPInfoHeader *infoHeader)
// {
//     FILE *file = fopen(filename, "rb");
//     if (!file)
//         return NULL;
//     fread(header, sizeof(BMPHeader), 1, file);
//     fread(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     if (header->type != 0x4D42)
//     {
//         fclose(file);
//         return NULL;
//     }
//     unsigned char *data = (unsigned char *)malloc(infoHeader->imageSize);
//     fseek(file, header->offset, SEEK_SET);
//     fread(data, infoHeader->imageSize, 1, file);
//     fclose(file);
//     return data;
// }

// void saveBMP(const char *filename, BMPHeader *header, BMPInfoHeader *infoHeader, unsigned char *data)
// {
//     FILE *file = fopen(filename, "wb");
//     fwrite(header, sizeof(BMPHeader), 1, file);
//     fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);
//     fseek(file, header->offset, SEEK_SET);
//     fwrite(data, infoHeader->imageSize, 1, file);
//     fclose(file);
// }

// int clamp(double x, int minVal, int maxVal)
// {
//     return (int)(x < minVal ? minVal : (x > maxVal ? maxVal : x));
// }

// // Helper functions to replace max and min
// int max(int a, int b)
// {
//     return (a > b) ? a : b;
// }

// int min(int a, int b)
// {
//     return (a < b) ? a : b;
// }
// double cubicInterpolate(double p[4], double x)
// {
//     return p[1] + 0.5 * x * (p[2] - p[0] + x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] + x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
// }

// double interpolate(double arr[4], double x)
// {
//     double vals[4] = {
//         cubicInterpolate(arr, x),
//         cubicInterpolate(arr + 1, x),
//         cubicInterpolate(arr + 2, x),
//         cubicInterpolate(arr + 3, x)};
//     return cubicInterpolate(vals, x);
// }

// // void bicubicInterpolate(const unsigned char *input, unsigned char *output, int width, int height, int newWidth, int newHeight)
// // {
// //     double scale_x = (double)width / newWidth;
// //     double scale_y = (double)height / newHeight;
// //     double frac_x, frac_y;
// //     int x, y, px, py;

// //     for (int i = 0; i < newHeight; ++i)
// //     {
// //         for (int j = 0; j < newWidth; ++j)
// //         {
// //             x = (int)(scale_x * j);
// //             y = (int)(scale_y * i);
// //             frac_x = scale_x * j - x;
// //             frac_y = scale_y * i - y;

// //             double patch[4][4] = {{0}};
// //             // for (int m = -1; m <= 2; ++m) {
// //             //     for (int n = -1; n <= 2; ++n) {
// //             //         px = min(max(x + n, 0), width - 1);  // Ensure px is within horizontal bounds
// //             //         py = min(max(y + m, 0), height - 1); // Ensure py is within vertical bounds
// //             //         patch[m + 1][n + 1] = input[py * width + px];
// //             //     }
// //             // }

// //             for (int m = -1; m <= 2; ++m)
// //             {
// //                 for (int n = -1; n <= 2; ++n)
// //                 {
// //                     int px = x + n;
// //                     int py = y + m;
// //                     if (px < 0 || px >= width || py < 0 || py >= height)
// //                     {
// //                         patch[m + 1][n + 1] = 0; // Use boundary conditions or extrapolation logic here if necessary
// //                     }
// //                     else
// //                     {
// //                         patch[m + 1][n + 1] = input[py * width + px];
// //                     }
// //                 }
// //             }
// //             double col[4] = {0};
// //             for (int k = 0; k < 4; k++)
// //             {
// //                 col[k] = interpolate(patch[k], frac_x);
// //             }
// //             output[i * newWidth + j] = (unsigned char)clamp(interpolate(col, frac_y), 0, 255);
// //         }
// //     }
// // }
// void bicubicInterpolate(const unsigned char* input, unsigned char* output, int width, int height, int newWidth, int newHeight) {
//     double scale_x = (double)width / newWidth;
//     double scale_y = (double)height / newHeight;
//     double frac_x, frac_y;
//     int x, y, px, py;

//     for (int i = 0; i < newHeight; ++i) {
//         for (int j = 0; j < newWidth; ++j) {
//             x = (int)(scale_x * j);
//             y = (int)(scale_y * i);
//             frac_x = scale_x * j - x;
//             frac_y = scale_y * i - y;

//             double patch[4][4];
//             // Fill the patch with boundary-safe pixel values
//             for (int m = -1; m <= 2; ++m) {
//                 for (int n = -1; n <= 2; ++n) {
//                     px = min(max(x + n, 0), width - 1);
//                     py = min(max(y + m, 0), height - 1);
//                     patch[m + 1][n + 1] = input[py * width + px];
//                 }
//             }

//             double col[4];
//             // Interpolate along x for each row in patch
//             for (int k = 0; k < 4; k++) {
//                 col[k] = interpolate(patch[k], frac_x);
//             }
//             // Interpolate the results from the above step along y
//             output[i * newWidth + j] = (unsigned char)clamp(interpolate(col, frac_y), 0, 255);
//         }
//     }
// }


// void applyConvolution(const unsigned char *input, unsigned char *output, int width, int height, const int kernel[3][3], int kernelDiv)
// {
// #pragma omp parallel for collapse(2)
//     for (int y = 1; y < height - 1; y++)
//     {
//         for (int x = 1; x < width - 1; x++)
//         {
//             int sum = 0;
//             for (int ky = -1; ky <= 1; ky++)
//             {
//                 for (int kx = -1; kx <= 1; kx++)
//                 {
//                     int idx = (y + ky) * width + (x + kx);
//                     sum += input[idx] * kernel[ky + 1][kx + 1];
//                 }
//             }
//             output[y * width + x] = (unsigned char)max(0, min(255, sum / kernelDiv));
//         }
//     }
// }

// int main(int argc, char *argv[])
// {
//     if (argc != 4)
//     {
//         printf("Usage: %s <input.bmp> <output.bmp> <num_threads>\n", argv[0]);
//         return 1;
//     }

//     int num_threads = atoi(argv[3]);
//     omp_set_num_threads(num_threads);

//     BMPHeader header;
//     BMPInfoHeader infoHeader;
//     unsigned char *inputData = loadBMP(argv[1], &header, &infoHeader);
//     if (!inputData)
//     {
//         printf("Failed to load image\n");
//         return 1;
//     }

//     int newWidth = infoHeader.width * 2; // Upscale factor of 2
//     int newHeight = infoHeader.height * 2;
//     // Instead of malloc, use calloc to initialize memory to zero
//     unsigned char *tempData = (unsigned char *)calloc(newWidth * newHeight, infoHeader.bitCount / 8);
//     unsigned char *outputData = (unsigned char *)calloc(newWidth * newHeight, infoHeader.bitCount / 8);

//     if (tempData == NULL || outputData == NULL)
//     {
//         fprintf(stderr, "Failed to allocate memory for image data\n");
//         free(inputData);
//         if (tempData != NULL)
//             free(tempData);
//         if (outputData != NULL)
//             free(outputData);
//         return 1;
//     }

//     double start_time = omp_get_wtime();
//     bicubicInterpolate(inputData, tempData, infoHeader.width, infoHeader.height, newWidth, newHeight);
//     // Define an edge detection kernel and apply it using the generic convolution function
//     int edgeKernel[3][3] = {
//         {-1, -1, -1},
//         {-1, 8, -1},
//         {-1, -1, -1}};
//     int kernelDiv = 1; // Division factor for kernel normalization, if needed
//     applyConvolution(tempData, outputData, newWidth, newHeight, edgeKernel, kernelDiv);

//     double end_time = omp_get_wtime();

//     infoHeader.width = newWidth;
//     infoHeader.height = newHeight;
//     infoHeader.imageSize = newWidth * newHeight * (infoHeader.bitCount / 8);

//     saveBMP(argv[2], &header, &infoHeader, outputData);

//     printf("Total processing time with OpenMP: %f seconds\n", end_time - start_time);

//     free(inputData);
//     free(tempData);
//     free(outputData);
//     return 0;
// }










