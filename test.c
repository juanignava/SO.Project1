#include <stdio.h>
#include <stdlib.h>
 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
 
const size_t NUM_PIXELS_TO_PRINT = 10U;

int getDecimal(int clave)
{
    int num = clave;
    int dec_value = 0;
 
    // Initializing base value to 1, i.e 2^0
    int base = 1;
 
    int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;
 
        dec_value += last_digit * base;
 
        base = base * 2;
    }
 
    return dec_value;
}

void buildImage(int width, int height, int channels, unsigned char *pixels, int j)
{
    unsigned char *img;
    printf("Aqui\n\n");
    for (int i = 0; i < j; i++){
        for (int j = 0; j < channels+1; j++){
            printf("%x", pixels[i]);
            img[i+j] = pixels[i]; 
        }
        printf("\n");
    }
    stbi_write_png("image2.png", width, height, channels, img, width*channels);
}

int main(void) {
    
    int width, height, channels;
    unsigned char *img = stbi_load("image.jpg", &width, &height, &channels, 0);

    if(img == NULL){
        printf("Error in loading");
        exit(1);
    }

    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    unsigned char *pixels;
    int j = 0;
    int primero = (width*height);
    int segundo = primero*channels;
    for (size_t i = 0; i < 10; i++) {
        
        printf("%02x%s", img[i], ((i + 1) % channels) ? "" : "\n");
        pixels[j] = img[i];
        //printf("\n");
        j++;
    }

    buildImage(width, height, channels, pixels, j);

    stbi_image_free(img);



    /*
    if (data) {
        printf("width = %d, height = %d, comp = %d (channels)\n", width, height, comp);
        for (size_t i = 0; i < NUM_PIXELS_TO_PRINT * comp; i++) {
            printf("%02x%s", data[i], ((i + 1) % comp) ? "" : "\n");
        }
        printf("\n");
    }
    */
    return 0;
}