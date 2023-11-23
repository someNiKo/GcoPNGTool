#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <png.h>

// 定义一个结构体，用来存储一个像素的颜色值
typedef struct {
    unsigned char r; // 红色
    unsigned char g; // 绿色
    unsigned char b; // 蓝色
    unsigned char a; // 透明
} pixel;

// 定义一个全局变量来存储错误信息
char error_msg[100];

// 定义一个自定义的错误处理函数，用于将错误信息保存到全局变量中，并终止程序
void my_png_error(png_structp png_ptr, png_const_charp msg)
{
    strcpy(error_msg, msg); // 复制错误信息
    longjmp(png_jmpbuf(png_ptr), 1); // 跳转到setjmp的位置
}

// 定义一个自定义的警告处理函数，用于将警告信息打印到标准输出中
void my_png_warning(png_structp png_ptr, png_const_charp msg)
{
    printf("Warning: %s\n", msg); // 打印警告信息
}

// 定义一个函数，用于写入png文件，接受一个二维的像素数组，以及图像的宽度和高度
void write_png(char *file_name, int width, int height, pixel **pixels)
{
    FILE *fp = fopen(file_name, "wb"); // 以二进制模式打开文件
    if (!fp) // 如果打开失败，返回
    {
        return;
    }

    // 创建一个写入结构体，使用自定义的错误处理函数
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, my_png_error, my_png_warning);
    if (!png_ptr) // 如果创建失败，关闭文件并返回
    {
        fclose(fp);
        return;
    }

    // 创建一个信息结构体
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) // 如果创建失败，销毁写入结构体，关闭文件并返回
    {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return;
    }

    // 设置一个跳转点，用于错误处理
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        // 如果发生错误，打印错误信息，销毁写入结构体和信息结构体，关闭文件并返回
        printf("Error: %s\n", error_msg);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    // 初始化输出流
    png_init_io(png_ptr, fp);

    // 设置图像的信息，例如宽度，高度，颜色类型，位深等
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // 设置图像的像素数据，将二维数组转换为一维数组
    png_bytep *row_pointers = malloc(height * sizeof(png_bytep));
    for (int i = 0; i < height; i++)
    {
        row_pointers[i] = (png_bytep)pixels[i];
    }
    png_set_rows(png_ptr, info_ptr, row_pointers);

    // 写入png文件
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // 结束写入过程
    png_write_end(png_ptr, info_ptr);

    // 销毁写入结构体和信息结构体
    png_destroy_write_struct(&png_ptr, &info_ptr);

    // 释放内存
    free(row_pointers);

    // 关闭文件
    fclose(fp);
}

// 定义一个函数，用来读取一个png文件，并返回一个二维的结构体数组
pixel **read_png(char *filename, int *width, int *height) {
    // 打开文件
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Can not open file %s\n", filename);
        return NULL;
    }

    // 检查文件是否为png格式
    unsigned char header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        printf("File %s is not .png file\n", filename);
        fclose(fp);
        return NULL;
    }

    // 创建一个png结构体，用来存储png文件的信息
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Can not creat png struct\n");
        fclose(fp);
        return NULL;
    }

    // 创建一个png信息结构体，用来存储png文件的元数据
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        printf("Can not creat png message struct\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    // 设置错误处理
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    // 初始化输入
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    // 读取png文件的信息
    png_read_info(png_ptr, info_ptr);

    // 获取png文件的宽度、高度、位深、颜色类型
    int w = png_get_image_width(png_ptr, info_ptr);
    int h = png_get_image_height(png_ptr, info_ptr);
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    int color_type = png_get_color_type(png_ptr, info_ptr);

    // 根据颜色类型，设置不同的转换
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr); // 调色板转为RGB
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr); // 灰度扩展为8位
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr); // 透明度转为alpha
    }
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr); // 16位转为8位
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr); // 灰度转为RGB
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_set_bgr(png_ptr); // RGB转为BGR
    }

    // 更新png信息结构体
    png_read_update_info(png_ptr, info_ptr);

    // 分配内存，用来存储每一行的像素数据
    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * h);
    if (!row_pointers) {
        printf("malloc error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    // 读取每一行的像素数据
    for (int i = 0; i < h; i++) {
        row_pointers[i] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[i]) {
            printf("malloc error\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            for (int j = 0; j < i; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            fclose(fp);
            return NULL;
        }
        png_read_row(png_ptr, row_pointers[i], NULL);
    }

    // 分配内存，用来存储每一个像素的颜色值
    pixel **pixels = (pixel **)malloc(sizeof(pixel *) * h);
    if (!pixels) {
        printf("malloc error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        for (int i = 0; i < h; i++) {
            free(row_pointers[i]);
        }
        free(row_pointers);
        fclose(fp);
        return NULL;
    }

    // 遍历每一个像素，获取其颜色值，并存储在结构体数组中
    for (int i = 0; i < h; i++) {
        pixels[i] = (pixel *)malloc(sizeof(pixel) * w);
        if (!pixels[i]) {
            printf("mallor error\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            for (int j = 0; j < i; j++) {
                free(pixels[j]);
            }
            free(pixels);
            for (int j = 0; j < h; j++) {
                free(row_pointers[j]);
            }
            free(row_pointers);
            fclose(fp);
            return NULL;
        }
        for (int j = 0; j < w; j++) {
            pixels[i][j].r = row_pointers[i][j * 4 + 2]; // 红色
            pixels[i][j].g = row_pointers[i][j * 4 + 1]; // 绿色
            pixels[i][j].b = row_pointers[i][j * 4 + 0]; // 蓝色
            pixels[i][j].a = row_pointers[i][j * 4 + 3]; // 透明
        }
    }

    // 释放内存，关闭文件
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    for (int i = 0; i < h; i++) {
        free(row_pointers[i]);
    }
    free(row_pointers);
    fclose(fp);

    // 返回结果
    *width = w;
    *height = h;
    return pixels;
}

// 定义一个函数，用来释放结构体数组的内存
void free_pixels(pixel **pixels, int height) {
    for (int i = 0; i < height; i++) {
        free(pixels[i]);
    }
    free(pixels);
}

// 定义一个主函数，用来测试
int main() {
    // 读取一个png文件
    int width, height;
    char PNGFile[100] = {0};
    printf("Filename:\n");
    gets(PNGFile);
    pixel **pixels = read_png(PNGFile, &width, &height);
    if (!pixels) {
        printf("Can not read file\n");
        return -1;
    }

    //积分计算几何中心纵坐标
    int x_avarage = 0, y_avarage = 0;
    int y_BlackPixelNum = 0, x_BlackPixelNum_Helper = 0, y_BlackPixel_Position = 0;
    for (int i = 0; i < height; i++) {
        x_BlackPixelNum_Helper = 0;
        for (int j = 0; j < width; j++) {
            if(pixels[i][j].b <= 225 && pixels[i][j].g <= 225 && pixels[i][j].r <= 225)
                x_BlackPixelNum_Helper++;
        }
        y_BlackPixelNum += x_BlackPixelNum_Helper;
        y_BlackPixel_Position += (i+1) * x_BlackPixelNum_Helper;
    }
    if(y_BlackPixelNum != 0)
        y_avarage = y_BlackPixel_Position / y_BlackPixelNum;
    else y_avarage = 1;

    // 积分计算几何中心横坐标
    int x_BlackPixelNum = 0, y_BlackPixelNum_Helper = 0, x_BlackPixel_Position = 0;
    for (int j = 0; j < width; j++) {
        y_BlackPixelNum_Helper = 0;
        for (int i = 0; i < height; i++) {
            pixels[i][j].a = 100;
            if(pixels[i][j].b <= 225 && pixels[i][j].g <= 225 && pixels[i][j].r <= 225)
                y_BlackPixelNum_Helper++;
        }
        x_BlackPixelNum += y_BlackPixelNum_Helper;
        x_BlackPixel_Position += (j+1) * y_BlackPixelNum_Helper;
    }
    if(x_BlackPixelNum != 0)
        x_avarage = x_BlackPixel_Position / x_BlackPixelNum;  
    else x_avarage = 1; 

    // 画出红线表示中心坐标
    for(int i = 0; i < height; i++) {
        pixels[i][x_avarage - 1].b = 0;
        pixels[i][x_avarage - 1].g = 0;
        pixels[i][x_avarage - 1].r = 255;
    }
    for(int j = 0; j < width; j++) {
        pixels[y_avarage][j].b = 0;
        pixels[y_avarage][j].g = 0;
        pixels[y_avarage][j].r = 255;
    }

    //配置新的png文件名
    char newPNGFile[106] = {'s', 'i', 'g', 'n', 'e', 'd'};
    int i = 0;
    while(PNGFile[i] != '\0'){
        newPNGFile[i + 6] = PNGFile[i];
        i++;
    }

    // 写入新的png文件
    write_png(newPNGFile, width, height, pixels);

    // 打印坐标并且停留
    printf("The geometric center of the image is x:%d, y:%d\n", x_avarage, y_avarage);
    for(int j = 0; i < 1000000000; i++);

    // 打开signed<yourfilenema>.png
    char command[256];
    snprintf(command, sizeof(command), "eog %s", newPNGFile);
    system(command);

    // 释放内存
    free_pixels(pixels, height);

    return 0;
}
