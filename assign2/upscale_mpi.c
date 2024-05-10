#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1, reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
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

unsigned char* loadBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    fread(header, sizeof(BMPHeader), 1, file);
    fread(infoHeader, sizeof(BMPInfoHeader), 1, file);
    if (header->type != 0x4D42) {
        fclose(file);
        return NULL;
    }

    unsigned char* data = (unsigned char*)malloc(infoHeader->imageSize);
    fseek(file, header->offset, SEEK_SET);
    fread(data, infoHeader->imageSize, 1, file);
    fclose(file);

    return data;
}

void saveBMP(const char* filename, BMPHeader* header, BMPInfoHeader* infoHeader, unsigned char* data) {
    FILE* file = fopen(filename, "wb");
    fwrite(header, sizeof(BMPHeader), 1, file);
    fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);
    fseek(file, header->offset, SEEK_SET);
    fwrite(data, infoHeader->imageSize, 1, file);
    fclose(file);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0)
            printf("Usage: %s <input.bmp> <output.bmp>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char* inputData = NULL;
    if (rank == 0) {
        inputData = loadBMP(argv[1], &header, &infoHeader);
        if (!inputData) {
            printf("Failed to load image\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&infoHeader, sizeof(infoHeader), MPI_BYTE, 0, MPI_COMM_WORLD);

    int upscale_factor = 2; // Define your upscale factor
    int newWidth = infoHeader.width * upscale_factor;
    int newHeight = infoHeader.height * upscale_factor;
    int newRowSize = (newWidth * infoHeader.bitCount + 31) / 32 * 4;
    int newImageSize = newHeight * newRowSize;

    unsigned char* outputData = malloc(newImageSize);

    // Calculate the part of the image each process will handle
    int local_height = newHeight / size;
    int extra = newHeight % size;
    int local_start = rank * local_height + (rank < extra ? rank : extra);
    int local_end = local_start + local_height + (rank < extra ? 1 : 0);
    unsigned char* localData = (unsigned char*)malloc(local_height * newRowSize);

    // Scatter the input data
    MPI_Scatter(inputData, local_height * newRowSize, MPI_UNSIGNED_CHAR, localData, local_height * newRowSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    double start_time = MPI_Wtime();
    for (int y = local_start; y < local_end; y++) {
        for (int x = 0; x < newWidth; x++) {
            int oldX = x / upscale_factor;
            int oldY = y / upscale_factor;
            int oldIndex = (oldY * infoHeader.width + oldX) * 3;
            int newIndex = (y * newWidth + x) * 3;
            outputData[newIndex + 0] = localData[oldIndex + 0];
            outputData[newIndex + 1] = localData[oldIndex + 1];
            outputData[newIndex + 2] = localData[oldIndex + 2];
        }
    }
    double end_time = MPI_Wtime();

    MPI_Gather(localData, local_height * newRowSize, MPI_UNSIGNED_CHAR, outputData, local_height * newRowSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        infoHeader.width = newWidth;
        infoHeader.height = newHeight;
        infoHeader.imageSize = newImageSize;
        saveBMP(argv[2], &header, &infoHeader, outputData);
        printf("Processing time with MPI: %f seconds\n", end_time - start_time);
    }

    free(inputData);
    free(outputData);
    free(localData);
    MPI_Finalize();
    return 0;
}
