/*
 * NASCERR_ImageProcessing.h
 *
 *  Created on: Oct 15, 2024
 *      Author: danal
 */

#ifndef INC_NASCERR_IMAGEPROCESSING_H_
#define INC_NASCERR_IMAGEPROCESSING_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include "NASCERR_Memory_32bits.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Experiment.h"

#define C1 6.5025f  // Constantes para estabilizar o denominador
#define C2 58.5225f

//#define LESC_LOGO
//#define MATRIX
//#define BARBARA_16
//#define BARBARA_48
//#define BARBARA_96
#define BARBARA_192

#define TEST_SIZE 1

#ifdef LESC_LOGO
#define BLOCK_SIZE 4  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 148 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 148
#endif

#ifdef MATRIX
#define BLOCK_SIZE 2  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 10 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 10
#endif

#ifdef BARBARA_16 //IMG_0
#define BLOCK_SIZE 4  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 16 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 16
#endif

#ifdef BARBARA_48 //IMG_1
#define BLOCK_SIZE 4  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 48 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 48
#endif

#ifdef BARBARA_96 //IMG_2
#define BLOCK_SIZE 4  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 96 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 96
#endif

#ifdef BARBARA_192 //IMG_3
#define BLOCK_SIZE 4  // Tamanho do bloco 4x4 para IMG_0, 2x2 para IMG_1
#define MEM_SIZE 1024
#define WIDTH 192 // Tamanho da imagem 148x148 para IMG_0, 10x10 para IMG_1
#define HEIGHT 192
#endif

void store_image_in_blocks(int width, int height, uint8_t image[height][width]);

void reconstruct_image_from_blocks(int width, int height, uint8_t image[height][width]);

int is_valid_image_size(int width, int height);

int rle_compress(uint8_t *input, int size, uint8_t *output);

int rle_decompress(uint8_t *COMPR_IMG, int input_size, uint8_t *VETOR_IMG);

float calculate_mse(int width, int height, uint8_t original[height][width], uint8_t processed[height][width]);

float calculate_psnr(int width, int height, uint8_t original[height][width], uint8_t processed[height][width]);

void write_image_blocks(uint8_t *COMPR_IMG, int input_size, NASCERR_MODE ecc, nascerr_rpt repeat);

void nascerr_image_processing(NASCERR_CTRL command, int width, int height, uint8_t image[height][width]);

NASCERR_RESPONSE nascerr_run_experiment(NASCERR_CTRL command);

#endif /* INC_NASCERR_IMAGEPROCESSING_H_ */
