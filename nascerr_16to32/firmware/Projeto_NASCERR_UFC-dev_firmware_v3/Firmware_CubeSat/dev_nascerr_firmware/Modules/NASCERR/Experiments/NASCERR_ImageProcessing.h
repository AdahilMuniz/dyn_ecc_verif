/**
 * ******************************************************************************
 *  @file    : NASCERR_ImageProcessing.h
 *  @brief   : Header for NASCERR_ImageProcessing.c
 *             Image-based SRAM radiation experiment: writes real grayscale image
 *             data (optionally RLE-compressed) through the FPGA ECC layer and
 *             measures quality degradation via PSNR and SSIM.
 *  @author  : Danilo Alencar
 * ******************************************************************************
 */

#ifndef INC_NASCERR_IMAGEPROCESSING_H_
#define INC_NASCERR_IMAGEPROCESSING_H_

/*
 * ******************************************************************************
 *  Includes
 * ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "NASCERR_Control.h"
#include "NASCERR_Memory_32bits.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Experiment.h"

/*
 * ******************************************************************************
 *  Constants
 * ******************************************************************************
 */
#define IMG_PACK(hi, lo)    ((uint16_t)(((uint16_t)(hi) << 8u) | (lo)))
#define IMG_UNPACK_HI(w)    ((uint8_t)((w) >> 8u))
#define IMG_UNPACK_LO(w)    ((uint8_t)((w) & 0xFFu))

/* SSIM stabilization constants (L=255, k1=0.01, k2=0.03) */
#define IMG_C1  6.5025f
#define IMG_C2  58.5225f

/* Number of experiment repetitions per run (used for statistical averaging) */
#define IMG_TEST_SIZE  1u

/* Maximum words written to SRAM per iteration (matches SRAM block granularity) */
#define IMG_MEM_SIZE   1024u

/* Scan block size in pixels (NxN tile used during serialization) */
#define IMG_BLOCK_SIZE 4u

/*
 * ******************************************************************************
 *  Image size selection — define exactly one of these before including this header,
 *  or set the default below.
 * ******************************************************************************
 */

//#define IMG_SELECT_BARBARA_16
#define IMG_SELECT_BARBARA_48
//#define IMG_SELECT_BARBARA_96
//#define IMG_SELECT_BARBARA_192

#if defined(IMG_SELECT_BARBARA_16)
#  define IMG_WIDTH   16u
#  define IMG_HEIGHT  16u
#elif defined(IMG_SELECT_BARBARA_48)
#  define IMG_WIDTH   48u
#  define IMG_HEIGHT  48u
#elif defined(IMG_SELECT_BARBARA_96)
#  define IMG_WIDTH   96u
#  define IMG_HEIGHT  96u
#elif defined(IMG_SELECT_BARBARA_192)
#  define IMG_WIDTH   192u
#  define IMG_HEIGHT  192u
#else
#  error "NASCERR_ImageProcessing: no image size selected"
#endif

/*
 * ******************************************************************************
 *  Types
 * ******************************************************************************
 */

/**
 * @brief Selects the test image to use in the experiment.
 */
typedef enum
{
    IMG_BARBARA_16  = 0,  /* > 16x16 crop of the Barbara standard test image  */
    IMG_BARBARA_48  = 1,  /* > 48x48 crop of the Barbara standard test image  */
    IMG_BARBARA_96  = 2,  /* > 96x96 crop of the Barbara standard test image  */
    IMG_BARBARA_192 = 3,  /* > 192x192 crop of the Barbara standard test image */
} ImgSelect_t;

/**
 * @brief Compression mode applied to the image before writing to SRAM.
 */
typedef enum
{
    IMG_LOSSLESS = 0,  /* > RLE compression applied before write, decompressed after read */
    IMG_LOSS_FF  = 1,  /* > Raw pixel data written directly (no compression)             */
} ImgCompression_t;

/**
 * @brief Command struct for an image processing SRAM experiment.
 *        Passed to nascerr_image_run_experiment().
 */
typedef struct
{
    id_db            id;           /* > Unique experiment identifier                    */
    NASCERR_MODE     ecc;          /* > ECC mode to apply during SRAM write/read        */
    nascerr_rpt      repeat;       /* > Number of write/read cycles per experiment run  */
    ImgSelect_t      image;        /* > Which test image to use                         */
    ImgCompression_t compression;  /* > Compression mode                                */
} NASCERR_IMAGE_CMD;

/*
 * ******************************************************************************
 *  Exported functions
 * ******************************************************************************
 */

/* Image serialization / reconstruction */
void img_store_in_blocks(int width, int height, const uint8_t image[height][width]);
void img_reconstruct_from_blocks(int width, int height, uint8_t image[height][width]);
int  img_valid_size(int width, int height);

/* RLE compression */
int  img_rle_compress(const uint8_t *input, int size, uint8_t *output);
int  img_rle_decompress(const uint8_t *compressed, int compressed_size, uint8_t *decompressed);

/* Quality metrics */
float img_calculate_mse(int width, int height, const uint8_t original[height][width], const uint8_t processed[height][width]);
float img_calculate_psnr(int width, int height, const uint8_t original[height][width], const uint8_t processed[height][width]);
float img_calculate_ssim(int width, int height, const uint8_t original[height][width], const uint8_t processed[height][width]);

/* SRAM write/read for image blocks */
void img_write_blocks(uint8_t *data, int size, NASCERR_MODE ecc, nascerr_rpt repeat);

/* Core experiment pipeline */
void img_process(NASCERR_IMAGE_CMD cmd, int width, int height, const uint8_t image[height][width]);

/* Top-level entry point — returns a filled NASCERR_EXPERIMENT result */
NASCERR_EXPERIMENT nascerr_image_run_experiment(NASCERR_IMAGE_CMD cmd);

/* Flash-image experiment pipeline (image sourced from external W25N01G flash) */
uint8_t            nascerr_image_flash_write(NASCERR_SCHEDULER cmd);
NASCERR_EXPERIMENT nascerr_image_flash_read (NASCERR_SCHEDULER cmd, time_t time_delta);

#endif /* INC_NASCERR_IMAGEPROCESSING_H_ */
