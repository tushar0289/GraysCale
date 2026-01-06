#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t nearest_ascii_shade(float pixel_value, int ramp_length);

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
        "File Type: 0x%02X\n"
        "File Size: %u\n"
        "Reserved1: %u\n"
        "Reserved2: %u\n"
        "Offset: %u\n",
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
    if(image == NULL){
        printf("Memory allocation failed!\n");
        fclose(fp);
        return 1;
    }

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

    uint8_t **luminosity = malloc(dibHeader.height * sizeof(uint8_t *));
    if(luminosity == NULL){
        printf("Memory allocation failed!\n");
        fclose(fp);
        return 1;
    }

    for(int i = 0; i < dibHeader.height; i++)
        luminosity[i] = malloc(dibHeader.width * sizeof(uint8_t));

    double temp_lum;

    for(int row = 0; row < dibHeader.height; row++){
        for(int col = 0; col < dibHeader.width; col++){
            temp_lum = (0.299 * image[row][col].data[2]) + (0.587 * image[row][col].data[1]) + (0.114 * image[row][col].data[0]);
            if (temp_lum >= 255.0)
                luminosity[row][col] = 255;
            else
                luminosity[row][col] = (uint8_t) (temp_lum + 0.5);
        }
    }


    printf("\nPixel Data in RGB format\n\n");

    FILE *write = fopen("chain.txt", "w");
    if(write == NULL){
        printf("Error Opening the File\n");
        return 1;
    }

    printf("Pixel Data in Luminous value\n\n");

    FILE *lumi = fopen("luminance.txt", "w");
    if(lumi == NULL){
        printf("Error Opening the File\n");
        return 1;
    }

    for(int row = 0; row < dibHeader.height; row++){
        for(int col = 0; col < dibHeader.width; col++)
            fprintf(lumi, "%d ", luminosity[row][col]);
        fprintf(lumi, "\n");
    }


    FILE *fgray = fopen("grayscale.bmp", "wb");
    BMPFileHeader grayscaleFileHeader = bmpFileHeader;
    BMPInfoHeader grayscaleInfoHeader = dibHeader;

    grayscaleInfoHeader.bitCount = 8;
    grayscaleInfoHeader.size = 40;

    uint32_t grayscale_row_width= (grayscaleInfoHeader.width + 3) / 4 * 4;
    uint8_t grayscale_padding = grayscale_row_width - grayscaleInfoHeader.width;

    grayscaleInfoHeader.imageSize = grayscale_row_width * grayscaleInfoHeader.height;

    grayscaleFileHeader.offset = 1024 + sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    grayscaleFileHeader.size = grayscaleFileHeader.offset + grayscaleInfoHeader.imageSize;
    

    fwrite(&grayscaleFileHeader, sizeof(BMPFileHeader), 1, fgray);
    fwrite(&grayscaleInfoHeader, sizeof(BMPInfoHeader), 1, fgray);

    uint8_t colorTable[256][4];
    for(int i = 0; i < 256; i++){
        colorTable[i][0] = i;
        colorTable[i][1] = i;
        colorTable[i][2] = i;
        colorTable[i][3] = 0;
    }

    fwrite(colorTable, sizeof(uint8_t), 256 * 4, fgray);


    uint8_t gray_padding = 0;
    for(int row = 0; row < grayscaleInfoHeader.height; row++){
        int reversed_row = (grayscaleInfoHeader.height - 1) - row;
        for(int col = 0; col < grayscaleInfoHeader.width; col++)
            fwrite(&luminosity[reversed_row][col], 1, 1, fgray);
        for(int p = 0; p < grayscale_padding; p++)
            fwrite(&gray_padding, 1, 1, fgray);
    }

    printf("Grayscale File Header:\n"
        "File Type: 0x%02X\n"
        "File Size: %u\n"
        "Reserved1: %u\n"
        "Reserved2: %u\n"
        "Offset: %u\n",
    grayscaleFileHeader.type, grayscaleFileHeader.size, grayscaleFileHeader.reserved1, grayscaleFileHeader.reserved2, grayscaleFileHeader.offset);

    printf("\n");

    printf("Grayscale BMP Info Header:\n"
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
    grayscaleInfoHeader.size, grayscaleInfoHeader.width, grayscaleInfoHeader.height, grayscaleInfoHeader.planes, grayscaleInfoHeader.bitCount, grayscaleInfoHeader.compression, grayscaleInfoHeader.imageSize, grayscaleInfoHeader.xResolution, grayscaleInfoHeader.yResolution, grayscaleInfoHeader.colorUsed, grayscaleInfoHeader.colorImportant);

    fclose(fgray);


    // dither grayscale

    FILE *fdither = fopen("dither.bmp", "wb");

    BMPFileHeader ditherFileHeader = grayscaleFileHeader;
    fwrite(&ditherFileHeader, sizeof(BMPFileHeader), 1, fdither);
    BMPInfoHeader ditherInfoHeader = grayscaleInfoHeader;
    fwrite(&ditherInfoHeader, sizeof(BMPInfoHeader), 1, fdither);
    fwrite(colorTable, sizeof(uint8_t), 256 * 4, fdither);


    const char *ascii_ramp = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.";

    float **dither_buffer = (float **) malloc(ditherInfoHeader.height * sizeof(float *));
    for(int i = 0; i < ditherInfoHeader.height; i++){
        dither_buffer[i] = (float *) malloc(ditherInfoHeader.width * sizeof(float));
        for(int j = 0; j < ditherInfoHeader.width; j++)
            dither_buffer[i][j] = (float) luminosity[i][j];
    }

    int ramp_length = strlen(ascii_ramp);

    uint8_t dither_padding = 0;
    for(int row = 0; row < ditherInfoHeader.height; row++){

        int current_y = (ditherInfoHeader.height - 1) - row;
        int next_y = current_y - 1;

        for(int col = 0; col < ditherInfoHeader.width; col++){
            float old_pixel = dither_buffer[current_y][col];

            uint8_t quantized_val = nearest_ascii_shade(old_pixel, ramp_length);
            dither_buffer[current_y][col] = quantized_val;

            fwrite(&quantized_val, 1, 1, fdither);

            float error = old_pixel - (float) (quantized_val);

            if(col + 1 < ditherInfoHeader.width)
                dither_buffer[current_y][col + 1] += error * 7.0f / 16.0f;

            if(next_y >= 0){
                if(col - 1 >= 0)
                    dither_buffer[next_y][col - 1] += error * 3.0f/ 16.0f; 
                dither_buffer[next_y][col] += error * 5.0f/ 16.0f;
                if(col + 1 < ditherInfoHeader.width)
                    dither_buffer[next_y][col + 1] += error * 1.0f / 16.0f;
            }
        }
        for(int p = 0; p < grayscale_padding; p++)
            fwrite(&dither_padding, 1, 1, fdither);
    }

    FILE *fascii = fopen("ascii_img.txt", "wb");

    for(int row = 0; row < ditherInfoHeader.height; row++){

        int current_y = row;
        int next_y = current_y - 1;

        for(int col = 0; col < ditherInfoHeader.width; col++){
            float old_pixel = dither_buffer[current_y][col];

            uint8_t quantized_val = nearest_ascii_shade(old_pixel, ramp_length);

            if(row % 2 == 0){
                int ramp_idx = (int) ((float)quantized_val / 255.0f * (ramp_length - 1) + 0.5f);
                fputc(ascii_ramp[ramp_idx], fascii);
            }
            float error = old_pixel - (float) quantized_val;
        }
        if(row % 2 == 0)
            fputc('\n', fascii);
    }

    fclose(fascii);

    for(int i = 0; i < ditherInfoHeader.height; i++)
        free(dither_buffer[i]);
    free(dither_buffer);

    fclose(fdither);

    fclose(lumi);


    for(int row = 0; row < dibHeader.height; row++){
        for(int col = 0; col < dibHeader.width; col++)
            fprintf(write, "(%d, %d, %d) ", image[row][col].data[2], image[row][col].data[1], image[row][col].data[0]);
        fprintf(write, "\n");
    }
    fclose(write);


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

    for(int i = 0; i < dibHeader.height; i++){
        free(luminosity[i]);
        luminosity[i] = NULL;
    }
    free(luminosity);
    luminosity = NULL;

    free(pixelData);
    pixelData = NULL;

    fclose(fp);

    return 0;
}

uint8_t nearest_ascii_shade(float pixel_value, int ramp_length){
    if(pixel_value < 0)
        pixel_value = 0;
    if(pixel_value > 255)
        pixel_value = 255;
    
    int index = (int) (pixel_value / 255.0f * (ramp_length - 1) + 0.5f);

    return (uint8_t) (255.0f * index /(ramp_length - 1));
}