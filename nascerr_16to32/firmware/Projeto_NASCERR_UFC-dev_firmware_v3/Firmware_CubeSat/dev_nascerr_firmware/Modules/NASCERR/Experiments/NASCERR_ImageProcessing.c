/**
 * ******************************************************************************
 *  @file    : NASCERR_ImageProcessing.c
 *  @brief   : Image-based SRAM radiation experiment for the NASCERR payload.
 *
 *             Pipeline per run:
 *               1. Select test image (barbara crop, stored in flash as const array)
 *               2. Serialize 2D image into a linear vector via NxN tile scan
 *               3. Optionally RLE-compress the vector
 *               4. Write data to SRAM through the FPGA ECC layer
 *               5. Read back from SRAM
 *               6. RLE-decompress (if applied) and reconstruct the 2D image
 *               7. Compute PSNR + SSIM vs. the original
 *               8. Count pixel-level errors
 *               9. Package results into NASCERR_EXPERIMENT
 *
 *  @author  : Danilo Alencar
 *             Adapted from dev_exp_integration branch (NASCERR_ImageProcessing.c)
 * ******************************************************************************
 */

#include "NASCERR_ImageProcessing.h"
#include "NASCERR_Flash.h"
#include "NASCERR_DB.h"

/*
 * ******************************************************************************
 *  Test images — stored in flash (const), standard Barbara grayscale crops.
 *  Source: dev_exp_integration branch, NASCERR_ImageProcessing.c
 * ******************************************************************************
 */

static const uint8_t barbara_16[16][16] = {
		{113, 90, 107, 157, 134, 87, 127, 163, 119, 85, 91, 101, 84, 95, 133, 93},
		{21, 39, 123, 156, 135, 87, 113, 153, 132, 107, 63, 118, 91, 88, 120, 88},
		{91, 94, 105, 137, 137, 120, 137, 148, 144, 167, 54, 93, 120, 91, 103, 84},
		{108, 89, 106, 171, 192, 201, 194, 162, 145, 105, 64, 69, 84, 137, 112, 147},
		{165, 168, 185, 201, 193, 180, 173, 172, 131, 99, 70, 63, 119, 76, 92, 139},
		{141, 152, 205, 164, 148, 147, 169, 166, 136, 84, 47, 100, 123, 118, 94, 81},
		{138, 194, 159, 116, 153, 160, 184, 163, 133, 92, 78, 165, 98, 130, 105, 88},
		{213, 153, 107, 100, 148, 161, 145, 124, 128, 105, 69, 110, 147, 137, 104, 81},
		{135, 103, 86, 83, 150, 153, 78, 40, 61, 81, 119, 124, 133, 144, 122, 91},
		{93, 100, 82, 83, 145, 156, 123, 50, 72, 93, 126, 71, 117, 135, 109, 103},
		{85, 81, 57, 90, 135, 153, 167, 86, 69, 79, 38, 92, 152, 96, 76, 83},
		{82, 66, 58, 83, 105, 104, 140, 96, 62, 62, 44, 138, 126, 52, 80, 89},
		{107, 81, 96, 71, 94, 71, 126, 123, 88, 51, 77, 161, 84, 88, 165, 135},
		{107, 116, 90, 73, 99, 84, 86, 128, 110, 68, 129, 152, 48, 120, 161, 138},
		{90, 108, 79, 68, 121, 106, 89, 77, 117, 125, 160, 114, 36, 126, 139, 119},
		{67, 84, 62, 69, 120, 101, 111, 90, 117, 124, 149, 75, 59, 132, 116, 103},
};

static const uint8_t barbara_48[48][48] = {
		{131, 133, 143, 149, 145, 146, 157, 137, 144, 150, 147, 149, 151, 153, 101, 96, 100, 98, 95, 89, 154, 217, 155, 115, 134, 127, 83, 83, 90, 102, 106, 107, 85, 84, 93, 85, 85, 83, 86, 88, 87, 84, 98, 165, 152, 119, 50, 111},
		{146, 146, 141, 131, 119, 105, 87, 88, 145, 152, 149, 150, 152, 154, 95, 88, 100, 97, 96, 96, 166, 201, 156, 123, 141, 142, 112, 116, 104, 92, 86, 98, 124, 101, 98, 100, 90, 91, 98, 92, 102, 100, 116, 169, 134, 107, 55, 111},
		{79, 63, 50, 39, 32, 31, 30, 65, 149, 150, 149, 149, 153, 154, 99, 83, 96, 97, 97, 98, 158, 189, 145, 117, 144, 127, 120, 112, 61, 75, 73, 45, 78, 152, 97, 95, 80, 80, 92, 87, 102, 102, 107, 159, 123, 97, 59, 113},
		{28, 27, 27, 32, 37, 39, 52, 117, 159, 145, 147, 150, 155, 154, 109, 91, 95, 95, 99, 97, 150, 182, 148, 120, 142, 128, 120, 81, 71, 85, 81, 77, 39, 122, 156, 95, 79, 84, 83, 83, 95, 100, 105, 150, 118, 84, 66, 114},
		{36, 35, 33, 37, 38, 80, 123, 146, 153, 140, 144, 149, 154, 152, 112, 101, 97, 97, 102, 97, 143, 170, 164, 130, 142, 133, 109, 144, 82, 45, 71, 82, 52, 72, 161, 142, 82, 84, 74, 80, 94, 104, 109, 145, 116, 79, 75, 118},
		{44, 33, 35, 34, 42, 103, 115, 129, 135, 144, 142, 146, 151, 148, 111, 103, 99, 104, 104, 98, 135, 153, 168, 138, 147, 113, 125, 184, 149, 101, 76, 75, 50, 51, 130, 155, 135, 79, 73, 73, 93, 97, 116, 145, 113, 68, 81, 115},
		{43, 29, 27, 34, 72, 110, 109, 101, 130, 149, 145, 150, 153, 148, 104, 103, 105, 104, 101, 92, 132, 136, 145, 146, 155, 104, 159, 184, 170, 164, 122, 41, 31, 50, 117, 131, 163, 102, 69, 73, 86, 91, 115, 106, 106, 67, 71, 118},
		{42, 65, 119, 92, 83, 97, 110, 73, 137, 156, 149, 148, 142, 136, 98, 88, 90, 95, 105, 107, 136, 137, 118, 154, 157, 122, 182, 179, 168, 143, 102, 39, 26, 51, 111, 121, 146, 114, 94, 90, 91, 88, 112, 90, 111, 66, 50, 87},
		{102, 153, 166, 160, 92, 90, 109, 60, 122, 92, 101, 116, 136, 159, 153, 152, 164, 176, 186, 193, 180, 159, 155, 164, 156, 118, 127, 168, 171, 146, 106, 49, 24, 58, 109, 104, 127, 81, 120, 118, 102, 77, 92, 105, 129, 53, 94, 147},
		{166, 159, 152, 78, 58, 104, 95, 54, 130, 130, 161, 172, 178, 186, 191, 194, 197, 196, 195, 196, 186, 157, 175, 167, 157, 117, 107, 148, 99, 83, 93, 51, 23, 74, 100, 92, 95, 47, 142, 150, 133, 107, 94, 130, 127, 117, 122, 112},
		{144, 128, 58, 47, 102, 118, 60, 55, 157, 172, 183, 190, 186, 189, 192, 194, 194, 194, 193, 192, 186, 155, 170, 154, 150, 155, 148, 134, 58, 82, 69, 48, 33, 90, 83, 63, 54, 52, 129, 168, 151, 136, 128, 129, 117, 145, 159, 145},
		{101, 31, 26, 73, 109, 128, 101, 129, 160, 168, 187, 194, 197, 198, 199, 197, 194, 193, 190, 187, 184, 153, 173, 151, 134, 159, 173, 100, 92, 112, 61, 69, 60, 95, 55, 61, 69, 43, 101, 162, 143, 129, 100, 103, 123, 155, 170, 171},
		{83, 121, 146, 145, 138, 139, 172, 175, 169, 196, 203, 204, 205, 203, 201, 199, 195, 193, 189, 185, 181, 163, 185, 141, 124, 161, 138, 70, 104, 134, 107, 71, 70, 88, 36, 83, 104, 39, 63, 135, 99, 45, 41, 90, 114, 149, 164, 162},
		{189, 211, 204, 184, 174, 177, 179, 183, 190, 206, 205, 202, 196, 195, 193, 190, 185, 182, 178, 171, 167, 179, 186, 150, 128, 165, 92, 59, 116, 114, 77, 46, 76, 67, 31, 96, 149, 122, 139, 103, 33, 31, 62, 116, 108, 128, 140, 137},
		{205, 208, 188, 179, 197, 200, 200, 200, 197, 191, 189, 186, 183, 179, 174, 171, 168, 165, 161, 154, 181, 187, 174, 168, 134, 143, 111, 89, 109, 89, 58, 57, 69, 43, 29, 120, 149, 146, 137, 87, 70, 87, 85, 115, 107, 107, 111, 111},
		{182, 192, 176, 158, 150, 193, 197, 191, 187, 182, 174, 165, 162, 158, 155, 155, 152, 149, 142, 162, 200, 189, 158, 177, 140, 139, 115, 78, 94, 90, 62, 62, 45, 27, 53, 157, 127, 86, 126, 134, 98, 72, 85, 114, 94, 86, 86, 87},
		{132, 121, 131, 117, 101, 151, 211, 201, 201, 207, 173, 144, 146, 144, 144, 142, 142, 142, 146, 173, 165, 177, 153, 164, 153, 144, 163, 117, 89, 67, 52, 51, 41, 45, 65, 173, 112, 52, 115, 170, 137, 91, 84, 112, 89, 80, 80, 77},
		{122, 115, 105, 102, 153, 176, 189, 196, 199, 170, 127, 142, 147, 146, 147, 150, 151, 149, 177, 191, 176, 159, 169, 130, 146, 142, 117, 86, 51, 71, 83, 32, 69, 121, 104, 182, 115, 68, 99, 172, 161, 87, 95, 112, 91, 80, 81, 78},
		{127, 115, 104, 164, 198, 195, 199, 204, 158, 126, 99, 145, 153, 153, 154, 156, 153, 159, 195, 187, 175, 165, 158, 147, 112, 138, 115, 84, 80, 85, 42, 45, 90, 149, 173, 179, 107, 74, 91, 142, 169, 110, 102, 110, 90, 86, 82, 81},
		{95, 126, 178, 196, 191, 200, 203, 157, 127, 119, 77, 156, 157, 155, 154, 154, 150, 176, 196, 176, 166, 174, 162, 161, 143, 129, 125, 144, 111, 58, 73, 85, 101, 174, 192, 163, 103, 66, 76, 104, 148, 137, 100, 110, 89, 93, 89, 89},
		{134, 185, 199, 197, 209, 201, 146, 124, 133, 101, 67, 158, 153, 152, 152, 153, 152, 168, 180, 179, 167, 167, 166, 134, 129, 137, 110, 108, 90, 80, 90, 93, 71, 147, 178, 156, 140, 77, 76, 133, 137, 146, 107, 108, 93, 97, 93, 92},
		{205, 204, 203, 212, 199, 139, 115, 128, 132, 89, 63, 156, 151, 148, 148, 147, 155, 178, 169, 154, 173, 177, 168, 149, 140, 131, 130, 129, 115, 105, 93, 71, 62, 78, 135, 158, 169, 141, 90, 130, 131, 143, 118, 98, 97, 94, 88, 84},
		{208, 207, 212, 194, 127, 113, 113, 123, 131, 77, 60, 151, 146, 144, 142, 141, 163, 179, 164, 130, 104, 122, 159, 166, 157, 148, 133, 120, 100, 83, 70, 69, 67, 56, 81, 157, 169, 149, 134, 132, 136, 154, 140, 86, 88, 84, 79, 76},
		{206, 212, 191, 125, 105, 116, 107, 118, 122, 52, 63, 147, 143, 141, 138, 143, 168, 170, 148, 121, 98, 57, 56, 87, 111, 114, 110, 105, 90, 82, 78, 76, 80, 100, 91, 102, 166, 153, 136, 149, 143, 143, 117, 82, 86, 85, 83, 79},
		{215, 187, 119, 108, 107, 111, 105, 116, 100, 33, 67, 151, 145, 143, 141, 145, 166, 160, 137, 98, 81, 73, 53, 39, 42, 56, 79, 104, 96, 93, 93, 102, 108, 108, 113, 86, 132, 162, 138, 136, 145, 150, 139, 93, 91, 88, 86, 83},
		{176, 115, 104, 109, 101, 104, 99, 112, 77, 29, 66, 151, 150, 148, 145, 141, 161, 156, 122, 67, 49, 49, 61, 58, 44, 36, 42, 87, 51, 67, 111, 125, 120, 127, 131, 130, 125, 156, 152, 133, 133, 145, 155, 128, 95, 97, 94, 91},
		{108, 101, 103, 106, 101, 102, 97, 103, 60, 29, 65, 152, 150, 148, 143, 138, 164, 158, 105, 46, 40, 39, 55, 61, 57, 33, 89, 132, 35, 66, 124, 144, 139, 138, 137, 160, 128, 114, 157, 145, 127, 124, 138, 151, 117, 93, 97, 94},
		{102, 99, 100, 104, 97, 102, 94, 103, 64, 27, 64, 146, 140, 138, 124, 137, 170, 142, 92, 66, 44, 39, 51, 58, 57, 37, 124, 121, 39, 108, 156, 155, 142, 121, 114, 90, 29, 59, 144, 152, 134, 115, 112, 132, 146, 118, 97, 102},
		{93, 97, 99, 102, 90, 97, 87, 107, 72, 25, 65, 145, 142, 144, 109, 146, 165, 159, 170, 155, 99, 52, 45, 50, 54, 46, 140, 87, 40, 112, 152, 163, 123, 66, 39, 33, 93, 126, 132, 150, 137, 114, 109, 99, 105, 114, 109, 109},
		{86, 94, 98, 91, 82, 92, 82, 117, 75, 24, 68, 154, 153, 145, 112, 161, 166, 182, 181, 162, 128, 88, 53, 46, 46, 60, 144, 59, 46, 109, 126, 126, 47, 30, 31, 104, 144, 138, 132, 142, 133, 112, 101, 96, 86, 80, 87, 113},
		{91, 84, 102, 84, 74, 89, 77, 140, 81, 21, 71, 162, 164, 129, 107, 154, 169, 175, 169, 165, 150, 121, 80, 49, 41, 74, 128, 40, 56, 110, 83, 35, 31, 30, 64, 145, 139, 144, 133, 129, 123, 102, 87, 88, 90, 73, 59, 101},
		{94, 80, 92, 78, 78, 84, 73, 91, 57, 24, 71, 165, 165, 105, 112, 145, 162, 164, 162, 162, 150, 123, 109, 80, 40, 91, 111, 34, 65, 102, 56, 29, 36, 32, 117, 158, 157, 148, 133, 102, 106, 92, 78, 84, 92, 74, 66, 110},
		{74, 85, 78, 71, 68, 73, 69, 32, 33, 28, 68, 162, 140, 94, 110, 126, 150, 155, 152, 153, 138, 102, 81, 81, 35, 105, 98, 35, 72, 91, 45, 32, 28, 64, 144, 157, 151, 142, 123, 82, 71, 82, 75, 77, 88, 80, 81, 113},
		{69, 83, 69, 67, 66, 67, 52, 25, 34, 26, 69, 166, 123, 93, 97, 107, 136, 146, 148, 146, 132, 110, 91, 67, 30, 88, 74, 38, 72, 83, 40, 30, 30, 111, 134, 144, 149, 135, 112, 78, 43, 51, 60, 67, 88, 87, 67, 99},
		{132, 76, 64, 64, 61, 62, 87, 81, 44, 25, 71, 170, 113, 95, 76, 89, 112, 129, 140, 142, 131, 108, 97, 96, 48, 66, 57, 45, 77, 79, 36, 26, 48, 132, 131, 143, 146, 129, 102, 69, 35, 45, 69, 53, 89, 79, 61, 112},
		{123, 97, 61, 63, 58, 65, 123, 148, 63, 19, 66, 149, 101, 98, 67, 61, 84, 118, 128, 133, 137, 112, 102, 99, 85, 69, 51, 50, 78, 70, 33, 24, 82, 136, 136, 146, 151, 120, 81, 61, 29, 73, 127, 96, 82, 71, 99, 118},
		{116, 119, 77, 57, 56, 71, 114, 123, 57, 20, 63, 146, 98, 89, 94, 53, 65, 114, 124, 129, 137, 121, 104, 104, 104, 90, 40, 57, 76, 67, 30, 30, 114, 141, 147, 149, 152, 95, 74, 54, 32, 139, 165, 151, 139, 133, 122, 107},
		{115, 117, 109, 65, 50, 83, 125, 126, 57, 20, 64, 149, 91, 73, 102, 75, 54, 96, 120, 128, 132, 129, 119, 101, 102, 105, 50, 57, 76, 62, 24, 48, 142, 152, 146, 153, 140, 75, 72, 38, 68, 175, 167, 163, 159, 155, 146, 135},
		{113, 114, 119, 96, 54, 104, 127, 128, 58, 19, 62, 140, 79, 61, 97, 95, 61, 66, 102, 123, 132, 131, 122, 118, 105, 105, 84, 64, 75, 58, 19, 99, 162, 149, 140, 155, 115, 64, 59, 27, 109, 178, 164, 161, 154, 150, 146, 147},
		{109, 111, 114, 117, 85, 97, 125, 128, 59, 19, 63, 142, 84, 56, 90, 100, 88, 56, 79, 106, 126, 137, 130, 114, 113, 110, 102, 81, 73, 41, 43, 150, 152, 149, 153, 152, 89, 59, 43, 36, 143, 173, 160, 158, 155, 150, 143, 139},
		{106, 109, 112, 114, 115, 109, 118, 128, 58, 19, 64, 141, 115, 56, 82, 101, 103, 78, 53, 93, 112, 127, 136, 121, 111, 111, 114, 99, 48, 16, 102, 161, 150, 155, 154, 133, 73, 51, 32, 60, 157, 155, 153, 148, 146, 139, 132, 129},
		{99, 100, 104, 108, 110, 116, 117, 124, 56, 19, 66, 136, 139, 85, 68, 101, 111, 104, 63, 56, 91, 110, 129, 118, 104, 96, 93, 113, 81, 53, 150, 152, 150, 156, 150, 113, 59, 42, 30, 80, 160, 149, 148, 145, 142, 136, 131, 126},
		{89, 90, 94, 98, 101, 105, 110, 119, 55, 19, 65, 133, 130, 123, 69, 92, 112, 120, 107, 49, 49, 83, 115, 117, 100, 97, 120, 153, 94, 118, 161, 145, 152, 149, 138, 99, 45, 36, 33, 84, 152, 146, 144, 146, 139, 131, 128, 124},
		{86, 96, 103, 105, 104, 103, 101, 104, 49, 18, 60, 122, 114, 129, 114, 73, 106, 117, 121, 103, 40, 54, 94, 112, 100, 109, 147, 146, 76, 145, 151, 152, 156, 148, 123, 81, 40, 37, 34, 91, 150, 145, 142, 145, 138, 128, 121, 117},
		{83, 92, 102, 105, 106, 104, 102, 94, 46, 25, 63, 107, 103, 124, 144, 96, 76, 112, 118, 121, 98, 39, 44, 105, 99, 112, 154, 109, 95, 154, 149, 158, 154, 139, 110, 69, 36, 35, 38, 112, 140, 134, 129, 125, 122, 118, 113, 107},
		{56, 66, 76, 79, 82, 86, 92, 83, 50, 46, 81, 93, 105, 127, 132, 135, 87, 90, 118, 121, 125, 96, 32, 82, 106, 119, 157, 85, 128, 152, 154, 158, 149, 125, 96, 56, 34, 28, 72, 140, 132, 132, 127, 120, 117, 108, 107, 103},
		{54, 62, 67, 64, 66, 72, 74, 63, 51, 52, 84, 87, 110, 117, 117, 117, 118, 80, 98, 120, 119, 121, 86, 55, 94, 128, 146, 91, 148, 152, 156, 152, 142, 114, 80, 43, 29, 50, 126, 133, 129, 126, 120, 116, 112, 105, 104, 99},
		{52, 72, 86, 90, 90, 89, 77, 51, 38, 53, 72, 90, 107, 106, 107, 111, 103, 78, 70, 108, 117, 116, 120, 117, 117, 131, 134, 115, 102, 70, 117, 140, 128, 103, 65, 35, 36, 75, 116, 121, 119, 119, 116, 113, 109, 105, 103, 99},
};

/*
 * ******************************************************************************
 *  Module-private state — static buffers allocated once (not on stack)
 * ******************************************************************************
 */

/* Linearized pixel vector after tile-scan serialization */
static uint8_t  s_image_vector[IMG_HEIGHT * IMG_WIDTH];

/* Buffer for RLE-compressed data (worst case: 2x original size) */
static uint8_t  s_image_stored[2u * IMG_HEIGHT * IMG_WIDTH];

/* Reconstructed image after SRAM round-trip */
static uint8_t  s_image_result[IMG_HEIGHT][IMG_WIDTH];

/* 16-bit staging buffer for SRAM write/read (one chunk at a time) */
static uint16_t s_img_buffer[IMG_MEM_SIZE];

/* Byte staging buffer for external flash reads (flash-image experiment) */
static uint8_t  s_flash_chunk[IMG_MEM_SIZE];

/* Per-run quality and error statistics */
static float    s_psnr;
static float    s_ssim;
static uint32_t s_total_errors;
static uint32_t s_errors_corrected;
static uint32_t s_tick;

/*
 * ******************************************************************************
 *  Local helpers
 * ******************************************************************************
 */

static float img_calc_mean(int width, int height, const uint8_t img[height][width])
{
    float sum = 0.0f;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            sum += img[i][j];
    return sum / (float)(width * height);
}

static float img_calc_variance(int width, int height, const uint8_t img[height][width], float mean)
{
    float v = 0.0f;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float d = (float)img[i][j] - mean;
            v += d * d;
        }
    return v / (float)(width * height);
}

static float img_calc_covariance(int width, int height,
                                  const uint8_t img1[height][width],
                                  const uint8_t img2[height][width],
                                  float mean1, float mean2)
{
    float cov = 0.0f;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            cov += ((float)img1[i][j] - mean1) * ((float)img2[i][j] - mean2);
    return cov / (float)(width * height);
}

/* Counts pixels that differ between original and processed */
static uint32_t img_count_errors(int width, int height,
                                  const uint8_t orig[height][width],
                                  const uint8_t proc[height][width])
{
    uint32_t count = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (orig[i][j] != proc[i][j]) count++;
    return count;
}

/* Counts pixels that match (correctly read back) */
static uint32_t img_count_corrected(int width, int height,
                                     const uint8_t orig[height][width],
                                     const uint8_t proc[height][width])
{
    uint32_t count = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (orig[i][j] == proc[i][j]) count++;
    return count;
}

/*
 * ******************************************************************************
 *  Public functions
 * ******************************************************************************
 */

int img_valid_size(int width, int height)
{
    return (width % IMG_BLOCK_SIZE == 0 && height % IMG_BLOCK_SIZE == 0);
}

void img_store_in_blocks(int width, int height, const uint8_t image[height][width])
{
    if (!img_valid_size(width, height))
        return;

    int idx = 0;
    for (int i = 0; i < height; i += IMG_BLOCK_SIZE)
        for (int j = 0; j < width; j += IMG_BLOCK_SIZE)
            for (int bi = 0; bi < IMG_BLOCK_SIZE; bi++)
                for (int bj = 0; bj < IMG_BLOCK_SIZE; bj++)
                    s_image_vector[idx++] = image[i + bi][j + bj];
}

void img_reconstruct_from_blocks(int width, int height, uint8_t image[height][width])
{
    int idx = 0;
    for (int i = 0; i < height; i += IMG_BLOCK_SIZE)
        for (int j = 0; j < width; j += IMG_BLOCK_SIZE)
            for (int bi = 0; bi < IMG_BLOCK_SIZE; bi++)
                for (int bj = 0; bj < IMG_BLOCK_SIZE; bj++)
                    image[i + bi][j + bj] = s_image_vector[idx++];
}

int img_rle_compress(const uint8_t *input, int size, uint8_t *output)
{
    int i = 0, j = 0;
    while (i < size) {
        uint8_t value = input[i];
        uint8_t count = 1;
        while (i + 1 < size && input[i] == input[i + 1] && count < 255) {
            count++;
            i++;
        }
        output[j++] = value;
        output[j++] = count;
        i++;
    }
    return j;
}

int img_rle_decompress(const uint8_t *compressed, int compressed_size, uint8_t *decompressed)
{
    int i = 0, j = 0;
    const int limit = IMG_HEIGHT * IMG_WIDTH;
    while (i < compressed_size) {
        if (j >= limit)
            return -1;
        uint8_t value = compressed[i];
        uint8_t count = compressed[i + 1];
        for (int k = 0; k < count && j < limit; k++)
            decompressed[j++] = value;
        i += 2;
    }
    return j;
}

float img_calculate_mse(int width, int height,
                         const uint8_t original[height][width],
                         const uint8_t processed[height][width])
{
    float mse = 0.0f;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float d = (float)original[i][j] - (float)processed[i][j];
            mse += d * d;
        }
    return mse / (float)(width * height);
}

float img_calculate_psnr(int width, int height,
                          const uint8_t original[height][width],
                          const uint8_t processed[height][width])
{
    float mse = img_calculate_mse(width, height, original, processed);
    if (mse == 0.0f)
        return INFINITY;
    return 10.0f * log10f((255.0f * 255.0f) / mse);
}

float img_calculate_ssim(int width, int height,
                          const uint8_t original[height][width],
                          const uint8_t processed[height][width])
{
    float mean_o = img_calc_mean(width, height, original);
    float mean_p = img_calc_mean(width, height, processed);
    float var_o  = img_calc_variance(width, height, original, mean_o);
    float var_p  = img_calc_variance(width, height, processed, mean_p);
    float cov    = img_calc_covariance(width, height, original, processed, mean_o, mean_p);

    return ((2.0f * mean_o * mean_p + IMG_C1) * (2.0f * cov + IMG_C2))
         / ((mean_o * mean_o + mean_p * mean_p + IMG_C1) * (var_o + var_p + IMG_C2));
}

void img_write_blocks(uint8_t *data, int size, NASCERR_MODE ecc, nascerr_rpt repeat)
{
    uint32_t current = 0;
    while (size > (int)current) {
        int      remaining  = size - (int)current;
        uint32_t chunk_size = ((uint32_t)remaining >= IMG_MEM_SIZE) ? IMG_MEM_SIZE : (uint32_t)remaining;

        /* Pack bytes into 16-bit words for SRAM */
        for (uint32_t i = 0; i < chunk_size; i++)
            s_img_buffer[i] = (uint16_t)(data[current + i]);

        for (nascerr_rpt r = 0; r < repeat; r++) {
            nascerr_memory_write_image_buffer(s_img_buffer, ecc, 0u, chunk_size);
            nascerr_io_delay(1u);
            uint16_t *read_ptr = nascerr_memory_read_block(0u, ecc);
            for (uint32_t i = 0; i < chunk_size; i++)
                s_img_buffer[i] = read_ptr[i];
        }

        /* Unpack back to bytes for the caller's buffer */
        for (uint32_t i = 0; i < chunk_size; i++)
            data[current + i] = (uint8_t)(s_img_buffer[i]);

        current += chunk_size;
    }
}

void img_process(NASCERR_IMAGE_CMD cmd, int width, int height, const uint8_t image[height][width])
{
    img_store_in_blocks(width, height, image);

    int stored_size = 0;
    int decomp_result = 0;

    if (cmd.compression == IMG_LOSSLESS) {
        stored_size  = img_rle_compress(s_image_vector, width * height, s_image_stored);
        img_write_blocks(s_image_stored, stored_size, cmd.ecc, cmd.repeat);
        decomp_result = img_rle_decompress(s_image_stored, stored_size, s_image_vector);
        (void)decomp_result;
    } else {
        stored_size = width * height;
        img_write_blocks(s_image_vector, stored_size, cmd.ecc, cmd.repeat);
    }

    img_reconstruct_from_blocks(width, height, s_image_result);

    s_psnr            = img_calculate_psnr(width, height, image, s_image_result);
    s_ssim            = img_calculate_ssim(width, height, image, s_image_result);
    s_total_errors    = img_count_errors(width, height, image, s_image_result);
    s_errors_corrected = img_count_corrected(width, height, image, s_image_result);
}

NASCERR_EXPERIMENT nascerr_image_run_experiment(NASCERR_IMAGE_CMD cmd)
{
    NASCERR_EXPERIMENT result;
    memset(&result, 0, sizeof(result));

    int width = 0, height = 0;

    switch (cmd.image) {
    case IMG_BARBARA_16:
        width = 16; height = 16;
        s_tick = HAL_GetTick();
        img_process(cmd, width, height, barbara_16);
        s_tick = HAL_GetTick() - s_tick;
        break;

    case IMG_BARBARA_48:
        width = 48; height = 48;
        s_tick = HAL_GetTick();
        img_process(cmd, width, height, barbara_48);
        s_tick = HAL_GetTick() - s_tick;
        break;

    /* barbara_96 and barbara_192 pixel arrays to be added — see NASCERR_ImageProcessing.h */
    case IMG_BARBARA_96:
    case IMG_BARBARA_192:
    default:
        return result;
    }

    result.id                          = cmd.id;
    result.block                       = 0u;
    result.data_lenght                 = 1u;
    result.time_delta                  = (time_t)s_tick;
    result.number_errors               = s_total_errors;
    result.corrected_data              = s_errors_corrected;
    result.percent_error               = nascerr_experiment_synthesise_error_info(s_total_errors, s_errors_corrected);
    result.ecc_telemetry.ecc           = cmd.ecc;
    result.ecc_telemetry.ecc_status    = (s_total_errors == 0u) ? ECC_OK : ECC_WARN;
    result.ecc_telemetry.uncorrectable_errors = s_total_errors;

    return result;
}

/* ============================================================================
 *  Flash-image experiment helpers
 * ============================================================================ */

/*
 * Counts the number of set bits in a 16-bit word (Hamming weight).
 */
static uint32_t img_popcount16(uint16_t v)
{
    uint32_t x = (uint32_t)v;
    x = x - ((x >> 1u) & 0x5555u);
    x = (x & 0x3333u) + ((x >> 2u) & 0x3333u);
    x = (x + (x >> 4u)) & 0x0F0Fu;
    return (x * 0x0101u) >> 8u;
}

/*
 ******************************************************************************
 *  Function Name : nascerr_image_flash_write
 *  @brief        : Reads the image stored in the external W25N01G flash
 *                  (address and size from experimentHeader.image_data) in
 *                  chunks of IMG_MEM_SIZE bytes, packs each byte into a
 *                  uint16_t word, and writes the resulting buffer to SRAM
 *                  through the FPGA ECC layer selected by cmd.ecc.
 *                  Writing starts at SRAM word offset 0 and advances until
 *                  the whole image is stored.
 *  @param        : cmd – NASCERR_SCHEDULER command; ecc field selects the
 *                         FPGA mode (TBEC, MRSC, BYPASS_1, …).
 *  @retval       : 1 on success, 0 if no valid image or a flash read fails.
 ******************************************************************************
 */
uint8_t nascerr_image_flash_write(NASCERR_SCHEDULER cmd)
{
    if (experimentHeader.image_data.valid != 0xAAu) {
        DEBUG_PRINT(LOG_WARN, "[IMG_FLASH_WRITE] No valid image in flash\r\n");
        return false;
    }

    uint32_t img_start = experimentHeader.image_data.header.start_addr + 16u;
    uint32_t img_size  = experimentHeader.image_data.image_size - 16u;

    /* 2 bytes por word → capacidade em bytes = sram_words * 2 */
    const uint32_t sram_max_bytes =
        (NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS);
    if (img_size > sram_max_bytes) {
        DEBUG_PRINT(LOG_WARN,
            "[IMG_FLASH_WRITE] Image %lu bytes > SRAM %lu, truncating\r\n",
            img_size, sram_max_bytes);
        img_size = sram_max_bytes;
    }

    /* Se img_size é ímpar, o último word terá só 1 pixel válido no byte alto */
    uint32_t num_words = (img_size + 1u) / 2u;

    uint32_t byte_offset = 0u;
    uint32_t word_offset = 0u;

    const uint8_t is_ecc_mode = (cmd.ecc == TBEC || cmd.ecc == MRSC);

    while (byte_offset < img_size) {
        uint32_t chunk_bytes = ((img_size - byte_offset) < (uint32_t)IMG_MEM_SIZE) ? (img_size - byte_offset) : (uint32_t)IMG_MEM_SIZE;

        FLASHTypeDef rd = nascerr_flash_readAddress(img_start + byte_offset, s_flash_chunk, chunk_bytes);
        if (rd.error) {
            DEBUG_PRINT(LOG_ERROR,
                "[IMG_FLASH_WRITE] Flash read error at offset %lu\r\n", byte_offset);
            return false;
        }

        /* Empacota 2 bytes por word */
        uint32_t chunk_words = (chunk_bytes + 1u) / 2u;
        for (uint32_t i = 0u; i < chunk_words; i++) {
            uint8_t hi = s_flash_chunk[i * 2u];
            uint8_t lo = (i * 2u + 1u < chunk_bytes) ? s_flash_chunk[i * 2u + 1u] : 0u;
            uint16_t packed = IMG_PACK(hi, lo);
            if (!is_ecc_mode) {
                packed = nascerr_inject_error_u16(word_offset + i, packed);
            }
            s_img_buffer[i] = packed;
        }

        nascerr_memory_write_image_buffer(s_img_buffer, cmd.ecc, word_offset, chunk_words);

        byte_offset += chunk_bytes;
        word_offset += chunk_words;
        i2c_slave_noBlockBus();
    }

    if (is_ecc_mode) {
        nascerr_memory_inject_postenc_errors(num_words);
        DEBUG_PRINT(LOG_INFO,
            "[IMG_FLASH_WRITE] Post-encoding error injection applied to %lu words\r\n",
            num_words);
    }

    DEBUG_PRINT(LOG_INFO,
        "[IMG_FLASH_WRITE] Done: %lu bytes → %lu words written to SRAM (ecc=%d)\r\n",
        img_size, num_words, (int)cmd.ecc);
    return true;
}


/*
 ******************************************************************************
 *  Function Name : nascerr_image_ecc_avaliation
 *  @brief        : ECC read/evaluation flow (TBEC / MRSC). Reads the image
 *                  back from SRAM word-by-word (2 packed pixels per word) and:
 *                    - encodes the flash reference to obtain expected
 *                      data + syndrome banks;
 *                    - counts raw bit flips in the data bank (1/2/3+ bit and
 *                      by region) against the expected permuted data;
 *                    - cross-checks the FPGA decoded word against a software
 *                      decode to flag FPGA/SW divergence;
 *                    - measures PSNR before (data_raw) and after (ecc_word)
 *                      correction, at pixel scale.
 *  @param        : cmd       - scheduler command (cmd.ecc = TBEC or MRSC).
 *  @param[out]   : result    - experiment struct to populate.
 *  @param        : img_start - flash byte address of the image.
 *  @param        : img_size  - image size in bytes (already clamped).
 *  @retval       : None
 ******************************************************************************
 */
static void nascerr_image_ecc_avaliation(NASCERR_SCHEDULER   cmd,
                                         NASCERR_EXPERIMENT *result,
                                         uint32_t            img_start,
                                         uint32_t            img_size)
{
    uint32_t ecc_fail_words = 0u;   /* FPGA ECC output != flash ref          */
    uint32_t bypass_bit_1   = 0u;   /* data-bank words with 1-bit flip       */
    uint32_t bypass_bit_2   = 0u;   /* data-bank words with 2-bit flip       */
    uint32_t bypass_bit_3p  = 0u;   /* data-bank words with 3+ bit flips     */
    uint32_t synd_only      = 0u;   /* flips only in syndrome bank           */
    uint32_t unmatch        = 0u;   /* SW decode != FPGA decode              */
    float    mse_raw_accum  = 0.0f;
    float    mse_ecc_accum  = 0.0f;
    uint32_t total_words    = 0u;
    uint32_t total_pixels   = 0u;

    volatile uint16_t *sram = (volatile uint16_t *)NASCERR_SRAM_ADDRESS;

    uint32_t byte_offset = 0u;
    uint32_t word_offset = 0u;

    while (byte_offset < img_size) {

        uint32_t chunk_bytes =
            ((img_size - byte_offset) < (uint32_t)IMG_MEM_SIZE)
            ? (img_size - byte_offset)
            : (uint32_t)IMG_MEM_SIZE;

        uint32_t chunk_words = (chunk_bytes + 1u) / 2u;

        FLASHTypeDef rd = nascerr_flash_readAddress(img_start + byte_offset,
                                                     s_flash_chunk,
                                                     chunk_bytes);
        if (rd.error) {
            DEBUG_PRINT(LOG_ERROR,
                "[IMG_ECC] Flash read error at offset %lu\r\n", byte_offset);
            break;
        }

        for (uint32_t i = 0u; i < chunk_words; i++) {

            uint8_t  ref_hi   = s_flash_chunk[i * 2u];
            uint8_t  ref_lo   = (i * 2u + 1u < chunk_bytes)
                                ? s_flash_chunk[i * 2u + 1u] : 0u;
            uint16_t ref_word = IMG_PACK(ref_hi, ref_lo);
            uint32_t sram_i   = word_offset + i;

            /* FPGA ECC-decoded output */
            nascerr_select_ecc(cmd.ecc);
            uint16_t ecc_word = *(sram + sram_i);

            /* Raw banks */
            nascerr_select_ecc(BYPASS_1);
            uint16_t bp1_word = *(sram + sram_i);  /* data bank     */

            nascerr_select_ecc(BYPASS_2);
            uint16_t bp2_word = *(sram + sram_i);  /* syndrome bank */

            if (ecc_word != ref_word)
                ecc_fail_words++;

            /* Expected codeword from the flash reference */
            uint32_t encoded   = nascerr_ecc_encode(ref_word, cmd.ecc);
            uint16_t exp_data  = (uint16_t)(encoded >> 16u);
            uint16_t exp_synd  = (uint16_t)(encoded & 0xFFFFu);

            uint16_t data_diff = bp1_word ^ exp_data;
            uint16_t synd_diff = bp2_word ^ exp_synd;

            /* Region-based classification of data-bank flips */
            nascerr_count_region(&result->ecc_telemetry, data_diff);

            /* Bit-flip count in data bank */
            uint32_t bits = img_popcount16(data_diff);
            if      (bits == 1u) bypass_bit_1++;
            else if (bits == 2u) bypass_bit_2++;
            else if (bits >= 3u) bypass_bit_3p++;

            /* Flip only in syndrome bank (data intact) */
            if (data_diff == 0u && synd_diff != 0u)
                synd_only++;

            /* SW decoder cross-check vs FPGA */
            uint32_t    raw_codeword = ((uint32_t)bp1_word << 16u) | bp2_word;
            ECC_DECODER sw_decoded   = nascerr_ecc_decode(raw_codeword, cmd.ecc);

            if (sw_decoded.data_corrected != ecc_word)
                unmatch++;

            /* Bit-error log over the full 32-bit codeword */
            uint32_t codeword_diff = encoded ^ raw_codeword;
            if (codeword_diff) {
                int8_t ecc_result = (ref_word == ecc_word) ? 0 : 1;
                print_bit_errors_ecc((uint32_t)(sram + sram_i), encoded, raw_codeword,
                                 codeword_diff, "SRAM_ECC", 32, ecc_result, ref_word, ecc_word);
            }

            /* Pixel-level MSE (2 pixels per word) */
            float draw_hi = (float)IMG_UNPACK_HI(sw_decoded.data_raw) - (float)ref_hi;
            float draw_lo = (float)IMG_UNPACK_LO(sw_decoded.data_raw) - (float)ref_lo;
            float decc_hi = (float)IMG_UNPACK_HI(ecc_word)            - (float)ref_hi;
            float decc_lo = (float)IMG_UNPACK_LO(ecc_word)            - (float)ref_lo;

            mse_raw_accum += draw_hi * draw_hi + draw_lo * draw_lo;
            mse_ecc_accum += decc_hi * decc_hi + decc_lo * decc_lo;

            total_pixels += 2u;
            total_words++;
        }

        byte_offset += chunk_bytes;
        word_offset += chunk_words;
        i2c_slave_noBlockBus();
    }

    /* --- Quality metrics --- */
    float mse_raw  = (total_pixels > 0u) ? (mse_raw_accum / (float)total_pixels) : 0.0f;
    float mse_ecc  = (total_pixels > 0u) ? (mse_ecc_accum / (float)total_pixels) : 0.0f;
    float psnr_raw = (mse_raw <= 0.0f) ? 9999.0f : 10.0f * log10f((255.0f * 255.0f) / mse_raw);
    float psnr_ecc = (mse_ecc <= 0.0f) ? 9999.0f : 10.0f * log10f((255.0f * 255.0f) / mse_ecc);

    uint32_t total_bypass_errors = bypass_bit_1 + bypass_bit_2 + bypass_bit_3p;

    DEBUG_PRINT(LOG_INFO,
        "[IMG_ECC] words=%lu  pixels=%lu  data_flips=%lu  synd_only=%lu"
        "  ecc_fail=%lu  unmatch=%lu\r\n"
        "          bp_1b=%lu  bp_2b=%lu  bp_3b+=%lu\r\n"
        "          PSNR_raw=%.2f dB  PSNR_ecc=%.2f dB\r\n",
        total_words, total_pixels, total_bypass_errors, synd_only,
        ecc_fail_words, unmatch,
        bypass_bit_1, bypass_bit_2, bypass_bit_3p, psnr_raw, psnr_ecc);

    /* --- Pack results --- */
//    result->data_lenght    = (nascerr_data)total_words;
    result->number_errors  = total_bypass_errors;
    result->corrected_data = (total_bypass_errors > ecc_fail_words)
                             ? (total_bypass_errors - ecc_fail_words) : 0u;
    result->percent_error  = nascerr_experiment_synthesise_error_info(
                                 result->number_errors, result->corrected_data);

    result->ecc_telemetry.single_bit_errors    = bypass_bit_1;
    result->ecc_telemetry.double_bit_errors    = bypass_bit_2;
    result->ecc_telemetry.triple_bit_errors    = bypass_bit_3p;
    result->ecc_telemetry.uncorrectable_errors = ecc_fail_words;

    if      (unmatch > 0u)             result->ecc_telemetry.ecc_status = ECC_ERR_UNKNOWN;
    else if (ecc_fail_words > 0u)      result->ecc_telemetry.ecc_status = ECC_FAIL;
    else if (total_bypass_errors > 0u) result->ecc_telemetry.ecc_status = ECC_WARN;
    else                               result->ecc_telemetry.ecc_status = ECC_OK;
}

/*
 ******************************************************************************
 *  Function Name : nascerr_image_bypass_avaliation
 *  @brief        : Bypass read/evaluation flow (BYPASS_1 / BYPASS_2). No ECC
 *                  is applied, so the word read from SRAM is the pixel data
 *                  directly. Counts raw bit flips (1/2/3+ bit and by region)
 *                  against the flash reference and measures PSNR.
 *  @param        : cmd       - scheduler command (cmd.ecc = BYPASS_1/2).
 *  @param[out]   : result    - experiment struct to populate.
 *  @param        : img_start - flash byte address of the image.
 *  @param        : img_size  - image size in bytes (already clamped).
 *  @retval       : None
 ******************************************************************************
 */
static void nascerr_image_bypass_avaliation(NASCERR_SCHEDULER   cmd,
                                            NASCERR_EXPERIMENT *result,
                                            uint32_t            img_start,
                                            uint32_t            img_size)
{
    uint32_t err_bit_1    = 0u;
    uint32_t err_bit_2    = 0u;
    uint32_t err_bit_3p   = 0u;
    float    mse_accum    = 0.0f;
    uint32_t total_words  = 0u;
    uint32_t total_pixels = 0u;

    const char *mem_name = (cmd.ecc == BYPASS_1) ? "SRAM1" : "SRAM2";

    volatile uint16_t *sram = (volatile uint16_t *)NASCERR_SRAM_ADDRESS;

    uint32_t byte_offset = 0u;
    uint32_t word_offset = 0u;

    while (byte_offset < img_size) {

        uint32_t chunk_bytes =
            ((img_size - byte_offset) < (uint32_t)IMG_MEM_SIZE)
            ? (img_size - byte_offset)
            : (uint32_t)IMG_MEM_SIZE;

        uint32_t chunk_words = (chunk_bytes + 1u) / 2u;

        FLASHTypeDef rd = nascerr_flash_readAddress(img_start + byte_offset,
                                                     s_flash_chunk,
                                                     chunk_bytes);
        if (rd.error) {
            DEBUG_PRINT(LOG_ERROR,
                "[IMG_BP] Flash read error at offset %lu\r\n", byte_offset);
            break;
        }

        for (uint32_t i = 0u; i < chunk_words; i++) {

            uint8_t  ref_hi   = s_flash_chunk[i * 2u];
            uint8_t  ref_lo   = (i * 2u + 1u < chunk_bytes)
                                ? s_flash_chunk[i * 2u + 1u] : 0u;
            uint16_t ref_word = IMG_PACK(ref_hi, ref_lo);
            uint32_t sram_i   = word_offset + i;

            /* Bypass read = pixel data directly (no permutation, no ECC) */
            nascerr_select_ecc(cmd.ecc);
            uint16_t rd_word = *(sram + sram_i);

            uint16_t diff = rd_word ^ ref_word;

            nascerr_count_region(&result->ecc_telemetry, diff);

            uint32_t bits = img_popcount16(diff);
            if      (bits == 1u) err_bit_1++;
            else if (bits == 2u) err_bit_2++;
            else if (bits >= 3u) err_bit_3p++;

            if (diff) {
                print_bit_errors((uint32_t)(sram+sram_i), (uint32_t)ref_word, (uint32_t)rd_word,
                                 (uint32_t)diff, mem_name, 16, -1);
            }

            /* Pixel-level MSE */
            float d_hi = (float)IMG_UNPACK_HI(rd_word) - (float)ref_hi;
            float d_lo = (float)IMG_UNPACK_LO(rd_word) - (float)ref_lo;
            mse_accum += d_hi * d_hi + d_lo * d_lo;

            total_pixels += 2u;
            total_words++;
        }

        byte_offset += chunk_bytes;
        word_offset += chunk_words;
        i2c_slave_noBlockBus();
    }

    /* --- Quality metrics --- */
    float mse  = (total_pixels > 0u) ? (mse_accum / (float)total_pixels) : 0.0f;
    float psnr = (mse <= 0.0f) ? 9999.0f : 10.0f * log10f((255.0f * 255.0f) / mse);

    uint32_t total_errors = err_bit_1 + err_bit_2 + err_bit_3p;

    DEBUG_PRINT(LOG_INFO,
        "[IMG_BP] mem=%s  words=%lu  pixels=%lu  flips=%lu"
        "  1b=%lu  2b=%lu  3b+=%lu  PSNR=%.2f dB\r\n",
        mem_name, total_words, total_pixels, total_errors,
        err_bit_1, err_bit_2, err_bit_3p, psnr);

    /* --- Pack results (no correction possible in bypass) --- */
//    result->data_lenght    = (nascerr_data)total_words;
    result->number_errors  = total_errors;
    result->corrected_data = 0u;
    result->percent_error  = nascerr_experiment_synthesise_error_info(
                                 result->number_errors, result->corrected_data);

    result->ecc_telemetry.single_bit_errors    = err_bit_1;
    result->ecc_telemetry.double_bit_errors    = err_bit_2;
    result->ecc_telemetry.triple_bit_errors    = err_bit_3p;
    result->ecc_telemetry.uncorrectable_errors = total_errors;

    result->ecc_telemetry.ecc_status = (total_errors > 0u) ? ECC_FAIL : ECC_OK;
}

/*
 ******************************************************************************
 *  Function Name : nascerr_image_flash_read
 *  @brief        : Dispatcher for the flash-image read experiment. Validates
 *                  the stored image header, clamps the size to SRAM capacity,
 *                  builds the result skeleton and routes to the ECC or bypass
 *                  evaluation flow according to cmd.ecc — mirroring the
 *                  tabular nascerr_experiment_read_result dispatch.
 *  @param        : cmd        - same NASCERR_SCHEDULER used for the write.
 *  @param        : time_delta - elapsed ms between write and this read.
 *  @retval       : NASCERR_EXPERIMENT populated with telemetry and metrics.
 ******************************************************************************
 */
NASCERR_EXPERIMENT nascerr_image_flash_read(NASCERR_SCHEDULER cmd, time_t time_delta)
{
    NASCERR_EXPERIMENT result  = {0};
    result.id                  = cmd.id;
    result.block               = 0xff;
    result.time_delta          = time_delta;
    result.ecc_telemetry.ecc   = cmd.ecc;
    result.data_lenght		   = 64; 			// Total de blocos presentes na SRAM

    /* --- Validate stored image --- */
    if (experimentHeader.image_data.valid != 0xAAu) {
        DEBUG_PRINT(LOG_ERROR, "[IMG_FLASH_READ] No valid image in flash\r\n");
        result.ecc_telemetry.ecc_status = ECC_FAIL;
        return result;
    }

    uint32_t img_start = experimentHeader.image_data.header.start_addr + 16u;
    uint32_t img_size  = experimentHeader.image_data.image_size - 16u;

    const uint32_t sram_max_bytes = (NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS);
    if (img_size > sram_max_bytes) {
        DEBUG_PRINT(LOG_WARN,
            "[IMG_FLASH_READ] Image %lu B > SRAM %lu B, truncating\r\n",
            img_size, sram_max_bytes);
        img_size = sram_max_bytes;
    }

    if (cmd.ecc == TBEC || cmd.ecc == MRSC)
        nascerr_image_ecc_avaliation(cmd, &result, img_start, img_size);
    else
        nascerr_image_bypass_avaliation(cmd, &result, img_start, img_size);

    i2c_slave_noBlockBus();
    return result;
}
