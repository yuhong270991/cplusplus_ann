#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "bmp.h"
#include <stdlib.h>
#include <stdio.h>
#include <helper_cuda.h>
#include <helper_functions.h>
using uint = unsigned int;

#define BLOCKSIZE 32
#define HALFBLOCK 16

#define BLOCKSIZEX 16
#define BLOCKSIZEY 16
#define BLOCKSIZEZ 4

__device__ float kernel_identity(float x);
__device__ float kernel_sigmoid(float x);
__device__ float kernel_tanh(float x);
__device__ float kernel_Relu(float x);


__global__ void simpleDotProduct(float* input, float* input2, float* output, uint length);

/*
	Parameter description:
	float* vector - array of float (1D) of size N
	float* matrix - array of float (2D) of size N
	float* output - array of float (1D) of size N
	uint length   - size N
*/
__global__ void dotProduct(float* inputMatrix, float* __restrict__ weightMatrix, float* output, uint length);



__global__ void feedForward(float* __restrict__ inputMatrix, float* __restrict__ weightMatrix,
							float* output, uint inputLength, uint inputHeight, uint matrixHeight, uint fnc);