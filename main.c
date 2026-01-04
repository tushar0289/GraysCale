#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct BMPFileHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct BMPInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xResolution;
    int32_t yResolution;
    uint32_t colorUsed;
    uint32_t colorImportant;
}BMPInfoHeader;
#pragma pack(pop)

typedef struct Pixel {
    uint8_t *data;
} Pixel;


int main(void){
    FILE *fp = fopen("chain.bmp", "rb");

    if(fp == NULL){
        printf("Error reading file!\n");
        return 1;
    }

    BMPFileHeader bmpFileHeader;

    size_t count = fread(&bmpFileHeader, sizeof(BMPFileHeader), 1, fp);

    if(count != 1){
        printf("Error Reading BMP Header!\n");
        fclose(fp);
        return 1;
    }

    /*
    uint16_t *typeAdd = &bmpFileHeader.type;
    char *ptr = (char *)typeAdd;
    */

    printf("BMP File Header:\n"
        "File Type: 0x%02X\n | "
        "File Size: %u\n | "
        "Reserved1: %u\n | "
        "Reserved2: %u\n | "
        "Offset: %u\n | ",
    bmpFileHeader.type, bmpFileHeader.size, bmpFileHeader.reserved1, bmpFileHeader.reserved2, bmpFileHeader.offset);

    printf("\n");

    BMPInfoHeader dibHeader;

    size_t count1 = fread(&dibHeader, sizeof(BMPInfoHeader), 1, fp);

    if(count1 != 1){
        printf("Unable to read info header!\n");
        fclose(fp);
        return 1;
    }

    printf("BMP Info Header:\n"
        "Size of the Info header: %u\n"
        "Width of the image: %d\n"
        "height of the image: %d\n"
        "Planes: %u\n"
        "Bit Count: %u\n"
        "Compression: %u\n"
        "Image Size: %u\n"
        "x Resolution: %d\n"
        "y Resolution: %d\n"
        "Color Used: %u\n"
        "Color Important: %u\n",
    dibHeader.size, dibHeader.width, dibHeader.height, dibHeader.planes, dibHeader.bitCount, dibHeader.compression, dibHeader.imageSize, dibHeader.xResolution, dibHeader.yResolution, dibHeader.colorUsed, dibHeader.colorImportant);

    Pixel **image = malloc(dibHeader.height * sizeof(Pixel *));
    for(int i = 0; i < dibHeader.height; i++)
        image[i] = malloc(dibHeader.width * sizeof(Pixel));

    uint8_t bytesPerPixel = dibHeader.bitCount / 8;

    uint32_t row_width = (dibHeader.width * bytesPerPixel + 3) / 4 * 4;
    uint8_t padding = row_width - (dibHeader.width * bytesPerPixel);

    fseek(fp, bmpFileHeader.offset, SEEK_SET);

    uint8_t *pixelData = malloc(bytesPerPixel);
    if(pixelData == NULL){
        printf("Failed to allocate memory!\n");
        fclose(fp);
        return 1;
    }

    for(int file_row = 0; file_row < dibHeader.height; file_row++){
        int img_row = (dibHeader.height - 1) - file_row;

        for(int col = 0; col < dibHeader.width; col++){
            fread(pixelData, 1, bytesPerPixel, fp);
            image[img_row][col].data = malloc(bytesPerPixel);
            for(int i = 0; i < bytesPerPixel; i++)
                image[img_row][col].data[i] = pixelData[i];
        }
        fseek(fp, padding, SEEK_CUR);
    }

    printf("\nPixel Data in RGB format\n\n");

    FILE *write = fopen("chain.txt", "w");
    if(write == NULL){
        printf("Error Opening the File\n");
        return 1;
    }


    for(int row = 0; row < dibHeader.height; row++){
        for(int col = 0; col < dibHeader.width; col++)
            fprintf(write, "%02X%02X%02X ", image[row][col].data[2], image[row][col].data[1], image[row][col].data[0]);
        fprintf(write, "\n");
    }

    for(int i = 0; i < dibHeader.height; i++){
        for(int j = 0; j < dibHeader.width; j++){
            free(image[i][j].data);
            image[i][j].data = NULL;
        }
        free(image[i]);
        image[i] = NULL;
    }
    free(image);
    image = NULL;

    free(pixelData);
    pixelData = NULL;

    fclose(fp);
    fclose(write);

    return 0;
}