#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include <math.h>
#include "stdbool.h"
#include <malloc.h>
#include <stdint.h>

typedef struct {
    uint16_t bfType; /**< BMP подпись */
    uint32_t bfSize; /**< BMP размер файла в байтах. */
    uint16_t bfReserved1, bfReserved2; /**< Зарезервированные поля. обычно равны 0 */
    uint32_t bfOffBits; /**< Смещение до самого изображения. */
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize; /**< Размер данной структуры в байтах,
                     указывающий также на версию структуры (здесь должно быть значение 40) */
    int32_t biWidth; /**< Ширина bitmap. */
    int32_t biHeight; /**< Высота of bitmap. */
    uint16_t biPlanes; /**< В BMP допустимо только значение 1. */
    uint16_t biBitCount; /**< Количество бит на пиксель. Допустимые значения: 1, 2, 4, 8, 16, 24, 32, 48, 64. */
    uint32_t biCompression; /**< Указывает на способ хранения пикселей. */
    uint32_t biSizeImage; /**< Размер пиксельных данных в байтах.
                          Может быть обнулено если хранение осуществляется двумерным массивом.*/
    int32_t biXPelsPerMeter; /**< Количество пикселей на метр по горизонтали. */
    int32_t biYPelsPerMeter; /**< Количество пикселей на метр по вертикали. */
    uint32_t biClrUsed; /**< Размер таблицы цветов в ячейках. */
    uint32_t biClrImportant; /**< Количество ячеек от начала таблицы цветов до последней используемой
                             (включая её саму). */
} BITMAPINFOHEADER;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t reserved;
} RGB_COLOR;

typedef union {
    RGB_COLOR rgb; /**< Представление BGRA цвета. */
    uint32_t value; /**< "Целое" представление цвета. */
} BITMAP_COLOR;

typedef struct {
    uint32_t width; /**< Ширины Bitmap. */
    uint32_t height; /**< Высота Bitmap. */
    BITMAP_COLOR *data; /**< Исходные пиксельные данные. */
} BITMAP;

uint32_t ReadUint32(FILE *fileS) {
    uint32_t valUint32;
    fread(&valUint32, sizeof(uint32_t), 1, fileS);
    return valUint32;
}

uint64_t ReadUint64(FILE *fileS) {
    uint64_t valUint64;
    fread(&valUint64, sizeof(uint64_t), 1, fileS);
    return valUint64;
}

uint16_t ReadUint16(FILE *fileS) {
    uint16_t valUint16;
    fread(&valUint16, sizeof(uint16_t), 1, fileS);
    return valUint16;
}

uint8_t ReadUint8(FILE *fileS) {
    uint8_t valUint8;
    fread(&valUint8, sizeof(uint8_t), 1, fileS);
    return valUint8;
}

void WriteUint32(FILE *fileS, uint32_t value) {
    fwrite(&value, sizeof(uint32_t), 1, fileS);
}

void WriteUint64(FILE *fileS, uint64_t value) {
    fwrite(&value, sizeof(uint64_t), 1, fileS);
}

void WriteUint16(FILE *fileS, uint16_t value) {
    fwrite(&value, sizeof(uint16_t), 1, fileS);
}

void WriteUint8(FILE *fileS, uint8_t value) {
    fwrite(&value, sizeof(uint8_t), 1, fileS);
}

int bmp_write(BITMAP *bitmap, const char *file);
void print_help();

BITMAP_COLOR bmp_color_sepia(BITMAP_COLOR a, uint8_t coef) {

    BITMAP_COLOR dst;

    dst.rgb.r = a.rgb.r * (1 - coef) +
            min((.393 * a.rgb.r) + (.769 * a.rgb.g) + (.189 * a.rgb.b), 255.0) * coef;
    dst.rgb.g = a.rgb.g * (1 - coef) +
                min((.349 * a.rgb.r) + (.686 * a.rgb.g) + (.168 * a.rgb.b), 255.0) * coef;
    dst.rgb.b = a.rgb.b * (1 - coef) +
                min((.272 * a.rgb.r) + (.534 * a.rgb.g) + (.131 * a.rgb.b), 255.0) * coef;

    return dst;
}

BITMAP *bmp_sepia(BITMAP *background, uint8_t coef) {

    if (background == NULL) {
        return NULL;
    }

    BITMAP *result = calloc(1, sizeof(BITMAP));
    result->width = background->width;
    result->height = background->height;
    result->data = calloc(result->width * result->height, sizeof(BITMAP_COLOR));
    for (int32_t y = 0; y < background->height; ++y) {
        for (int32_t x = 0; x < background->width; ++x) {
            BITMAP_COLOR a = background->data[y * result->width + x];
            result->data[y * result->width + x] = bmp_color_sepia(a, coef);
        }
    }
    return result;
}

BITMAP *bmp_read(const char *file) {

    FILE *bmp_file;
    BITMAP *bitmap;

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER *bitmapinfoheader = NULL;

    uint32_t bitmapinfo_size;

    if ((bmp_file = fopen(file, "rb")) == NULL) {
        return NULL;
    }

    file_header.bfType = ReadUint16(bmp_file);

    if (file_header.bfType != 0x4D42) {
        fclose(bmp_file);
        return NULL;
    }

    file_header.bfSize = ReadUint32(bmp_file);
    file_header.bfReserved1 = ReadUint16(bmp_file);
    file_header.bfReserved2 = ReadUint16(bmp_file);
    file_header.bfOffBits = ReadUint32(bmp_file);

    //Считывание BITMAPINFO's размера
    bitmapinfo_size = ReadUint32(bmp_file);

    //Проверка выделения памяти
    if ((bitmap = calloc(1, sizeof(BITMAP))) == NULL) {
        fclose(bmp_file);
        return NULL;
    }

    //Чтение

    if ((bitmapinfoheader = calloc(1, sizeof(BITMAPINFOHEADER))) == NULL) {
        free(bitmap);
        fclose(bmp_file);
        return NULL;
    }

    bitmapinfoheader->biSize = bitmapinfo_size;
    bitmapinfoheader->biWidth =  ReadUint32(bmp_file);
    bitmapinfoheader->biHeight = ReadUint32(bmp_file);
    bitmapinfoheader->biPlanes = ReadUint16(bmp_file);
    bitmapinfoheader->biBitCount = ReadUint16(bmp_file);
    bitmapinfoheader->biCompression = ReadUint32(bmp_file);
    bitmapinfoheader->biSizeImage = ReadUint32(bmp_file);
    bitmapinfoheader->biXPelsPerMeter = ReadUint32(bmp_file);
    bitmapinfoheader->biYPelsPerMeter = ReadUint32(bmp_file);
    bitmapinfoheader->biClrUsed = ReadUint32(bmp_file);
    bitmapinfoheader->biClrImportant = ReadUint32(bmp_file);

    bitmap->width = bitmapinfoheader->biWidth;
    bitmap->height = bitmapinfoheader->biHeight;

    free(bitmapinfoheader);

    fseek(bmp_file, file_header.bfOffBits, SEEK_SET);

    if ((bitmap->data = calloc(bitmap->width * bitmap->height, sizeof(BITMAP_COLOR))) == NULL) {
        free(bitmap);
        fclose(bmp_file);
        return NULL;
    }

    //Сканирование основных параметров битмапа
    for (int32_t y = bitmap->height - 1; y >= 0; --y) {
        uint32_t bytes_read = 0;
        for (int32_t x = 0; x < bitmap->width; ++x) {
            uint8_t pixels;
            BITMAP_COLOR color;
            color.rgb.b = ReadUint8(bmp_file);
            color.rgb.g = ReadUint8(bmp_file);
            color.rgb.r = ReadUint8(bmp_file);
            bytes_read += 3;
            bitmap->data[y * bitmap->width + x] = color;
        }

        //Размер строки изображения BMP должен быть делимым на 4,
        //поэтому обычно имеется несколько нечетных байтов.
        while (bytes_read % 4) {
            ReadUint8(bmp_file);
            ++bytes_read;
        }
    }
    fclose(bmp_file);
    return bitmap;
}

int bmp_write(BITMAP *bitmap, const char *file) {
    FILE *bmp_file;
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    //Сохранить в 24-битный BMP-файл
    if (bitmap == NULL) {
        return 1;
    }

    if ((bmp_file = fopen(file, "wb")) == NULL) {
        return 2;
    };

    uint32_t bitmapdata_size = ((bitmap->width % 4) ?
                                (bitmap->width + (4 - (bitmap->width % 4))) :
                                (bitmap->width))
                               * bitmap->height * 3;


    file_header.bfType = 0x4D42;
    file_header.bfSize = 14 + 40 + bitmapdata_size;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = 14 + 40;

    info_header.biSize = 40;
    info_header.biWidth = bitmap->width;
    info_header.biHeight = bitmap->height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = 0;
    info_header.biSizeImage = 0;
    info_header.biXPelsPerMeter = 0;
    info_header.biYPelsPerMeter = 0;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;

    WriteUint16(bmp_file, file_header.bfType);
    WriteUint32(bmp_file, file_header.bfSize);
    WriteUint16(bmp_file, file_header.bfReserved1);
    WriteUint16(bmp_file, file_header.bfReserved2);
    WriteUint32(bmp_file, file_header.bfOffBits);

    WriteUint32(bmp_file, info_header.biSize);
    WriteUint32(bmp_file, info_header.biWidth);
    WriteUint32(bmp_file, info_header.biHeight);
    WriteUint16(bmp_file, info_header.biPlanes);
    WriteUint16(bmp_file, info_header.biBitCount);
    WriteUint32(bmp_file, info_header.biCompression);
    WriteUint32(bmp_file, info_header.biSizeImage);
    WriteUint32(bmp_file, info_header.biXPelsPerMeter);
    WriteUint32(bmp_file, info_header.biYPelsPerMeter);
    WriteUint32(bmp_file, info_header.biClrUsed);
    WriteUint32(bmp_file, info_header.biClrImportant);

    for (int32_t y = bitmap->height - 1; y >= 0; --y) {
        uint32_t bytes_written = 0;
        for (int32_t x = 0; x < bitmap->width; ++x) {
            WriteUint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.b);
            WriteUint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.g);
            WriteUint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.r);
            bytes_written += 3;
        }
        //Размер строки изображения BMP должен быть делимым на 4,
        //поэтому обычно имеется несколько нечетных байтов.
        while (bytes_written % 4) {
            WriteUint8(bmp_file, 0);
            ++bytes_written;
        }
    }
    fclose(bmp_file);
    return 0;
}

int main(int argc, char **argv) {

    if (strcmp(argv[1],"--help") == 0) {
        print_help();

        return 0;
    }

    if (strcmp(argv[1], "sepia") == 0) {
        char *input = argv[2];
        float coef = atof(argv[3]);
        int opacity;

        BITMAP *BMP;
        BITMAP *sepia;

        coef *= 255;
        opacity = (int)coef;

        BMP = bmp_read(input);
        sepia = bmp_sepia(BMP, opacity);

        bmp_write(sepia, "sepia.bmp");
    }

    printf("Well done!\n");

return 0;
}

void print_help() {
    printf("Program works with 24bait files\n"
           "Type sepia <input name> <opacity coefficient>\n"
           "<Opacity coefficient> of opacity from 0 to 1\n"
           "Completed by Maxim Cherbadzhi\n");
    return;
}