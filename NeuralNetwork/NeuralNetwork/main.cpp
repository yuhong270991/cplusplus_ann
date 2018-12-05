#include "kernel.h"
#include "utils\MathUtils.h"
#include "ann/neuralnet.h"
#include <cmath>

template<typename T>
T mysigmoid(T val)
{
  assert(val >= (T)-709.f);
  return  1.f / (1.f + (T)std::exp(-val));
}

template<typename T>
T mysigmoidPrime(T val)
{
  T temp = mysigmoid(val);
  return temp * (1.f - temp);
}

template <typename T>
T mytanH(T val)
{
  return (T)std::tanh(val);
}

template <typename T>
T mytanHPrime(T val)
{
  T x = (T)std::tanh(val);
	return 1.f - (x * x);
}

void ann_cpu_test()
{
	//two neurons input, two neurons hidden, one neuron output
	std::vector<unsigned> config{ 2, 2, 1 };
	NeuralNet nn(config);

	//hardcoded XOR input and labels
	/*XOR TABLE
	0 0 0
	0 1 1
	1 0 1
	1 1 0
	*/

	std::vector<std::vector<float>> inputs = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 1 }
	};
	std::vector<float> labels = { 0, 1, 1, 0 };

	//hardcoded initialization of weights and bias for testing
	float i1_h1 = -0.7706f;
	float i2_h1 = 0.6257f;
	float h1_bias = 0.1859f;
	float i1_h2 = 0.5607f;
	float i2_h2 = 0.2109f;
	float h2_bias = -0.7984f;
	float h1_o1 = 0.5951f;
	float h2_o1 = 0.3433f;
	float o1_bias = 0.1328f;

	//set weights

	//input layer to first neuron in hidden layer
	nn.getNeuron(0, 0).setWeight(0, i1_h1);
	nn.getNeuron(0, 1).setWeight(0, i2_h1);
  nn.getNeuron(0, 2).setWeight(0, h1_bias);

  //input layer to second neuron in hidden layer
  nn.getNeuron(0, 0).setWeight(1, i1_h2);
  nn.getNeuron(0, 1).setWeight(1, i2_h2);
  nn.getNeuron(0, 2).setWeight(1, h2_bias);

	//hidden to output layer
	nn.getNeuron(1, 0).setWeight(0, h1_o1);
	nn.getNeuron(1, 1).setWeight(0, h2_o1);
  nn.getNeuron(1, 2).setWeight(0, o1_bias);

  //tanh is activation function for input and hidden layer
  nn.getNeuron(1, 0).setActivationFunctions(mytanH, mytanHPrime);
  nn.getNeuron(1, 1).setActivationFunctions(mytanH, mytanHPrime);

  //sigmoid is activation function for output layer
  nn.getNeuron(2, 0).setActivationFunctions(mysigmoid, mysigmoidPrime);

  nn.train(inputs, labels, 0.5f, 1);
  //nn.status();
  nn.printWeights();
}

int main(int argc, char **argv)
{
	ann_cpu_test();
#if 0
	//if (argc != 2) {
	//	printf("Usage: [image file]");
	//	return -1;
	//}

	//Test test
	std::vector<unsigned> test{ 2, 2, 1 };
	NeuralNet testNetwork(test);

	int numOfNeurons = 0;
	std::vector<int> hiddenLayerIndex;
	std::vector<float> weights;
	//skip output layer
	//for each layer
	for (int i = 0; i < test.size() - 1; ++i)
	{
		//for each neuron
		for (int j = 0; j < testNetwork.m_layers[i].size(); ++j)
		{
			Neuron temp = testNetwork.getNeuron(i, j);
			//for each synapse
			for (int k = 0; k < temp.m_edges.size(); ++k)
			{
				weights.push_back(temp.m_edges[k].weight);
			}
		}
	}

	std::vector<float> inputVal;
	for (int i = 0; i < test[0]; ++i)
	{
		//get us float* vector; the input
		inputVal.push_back(testNetwork.getNeuron(0, i).getOutput());
	}

	float * h_input = &inputVal[0];

	//offset outselves to get correct set of weights
	float * h_hiddenWeights = &weights[0];

	//get the sizes of the inputMatrix and weightMatrix's height.
	//weightMatrix size startVecLength * startMatHeight
	//going forward, we can push in test/config ptr into GPU then we access the length value directly from GPU call
	int startVecLength = test[0], startMatHeight = test[1];



	////weightMatrix for 1st layer
	//float * tempWeightMat = new float[numOfNeurons];
	//
	////update weightMatrix with synapse weight
	//for (int i = 0; i < numOfNeurons; ++i)
	//{
	//	for (int j = 0; j < testNetwork.m_layers[i].size(); ++j)
	//		tempWeightMat[i] = testNetwork.getNeuron(i, j).;
	//}

	//


	StopWatchInterface *hTimer = NULL;
	sdkCreateTimer(&hTimer);
	cudaDeviceProp deviceProp;
	deviceProp.major = 0;
	deviceProp.minor = 0;

	// For this ANN, we are only dealing with 3 layers
	//bmp_header header;
	//unsigned char* imageData;
	//float* h_input;
	//float* d_inLayer, d_outLayer;

	//Use command-line specified CUDA device, otherwise use device with highest Gflops/s
	int dev = findCudaDevice(argc, (const char **)argv);

	checkCudaErrors(cudaGetDeviceProperties(&deviceProp, dev));
	cudaGetDevice(&dev);

	printf("CUDA device [%s] has %d Multi-Processors, Compute %d.%d\n",
		deviceProp.name, deviceProp.multiProcessorCount, deviceProp.major, deviceProp.minor);

	//bmp_read(argv[1], &header, &imageData);
	//int imageWidth = header.width;
	//int imageHeight = header.height;
	//int imageChannels = 3;
	//double dAvgSecs;
	//uint byteCount = imageWidth * imageHeight * imageChannels * sizeof(unsigned char);

	/*
	----------------------------
	VECTOR TO MATRIX DOT PRODUCT
	----------------------------
	*/
	std::cout << "Starting GPU vector to matrix dot product: \n";
	std::cout << "Length: " << std::atoi(argv[1]) << std::endl;

	// YANWEN TEST
	float *d_arr1, *d_arr2, *h_arr1, *h_arr2;
	float *d_output, *h_output, *dh_output;
	const uint vmSize = std::atoi(argv[1]);
	const uint debugMode = std::atoi(argv[2]);
	h_arr1 = new float[vmSize * 12];
	h_arr2 = new float[vmSize * 12];
	h_output = new float[vmSize * 12];
	dh_output = new float[vmSize * 12];

	for (int i = 0; i < vmSize*12; ++i) {
		h_arr1[i] = debugMode == 1 ? 1 : 1.0f / (i + 1);
		h_output[i] = 0.0f;
		h_arr2[i] = 1;
	}


	checkCudaErrors(cudaMalloc((void **)&(d_arr1), vmSize * 12 * sizeof(float)));
	checkCudaErrors(cudaMalloc((void **)&(d_arr2), vmSize * 12 * sizeof(float)));
	checkCudaErrors(cudaMalloc((void **)&(d_output), vmSize * 12 * sizeof(float)));
	checkCudaErrors(cudaMemcpy(d_arr1, h_arr1, vmSize * 12 * sizeof(float), cudaMemcpyHostToDevice));
	checkCudaErrors(cudaMemcpy(d_arr2, h_arr2, vmSize * 12 * sizeof(float), cudaMemcpyHostToDevice));

	auto numberOfRows = (unsigned)ceilf(float(vmSize) / BLOCKSIZEX);
	auto numOfDataset = (unsigned)ceilf(float(12) / BLOCKSIZEZ);
	dim3 gridSize(numberOfRows, numberOfRows, numOfDataset);
	dim3 blockSize(BLOCKSIZEX, BLOCKSIZEY, BLOCKSIZEZ);
	sdkResetTimer(&hTimer);
	sdkStartTimer(&hTimer);

	//dotProduct << < gridSize, blockSize >> > (d_arr1, d_arr2, d_output, vmSize);
	feedForward << < gridSize, blockSize >> > (d_arr1, d_arr2, d_output, vmSize, 12, 12, 1);
	checkCudaErrors(cudaDeviceSynchronize());

	checkCudaErrors(cudaMemcpy(dh_output, d_output, vmSize * 12 * sizeof(float), cudaMemcpyDeviceToHost));

	sdkStopTimer(&hTimer);
	float GPUTime = 1.0e-3 * (double)sdkGetTimerValue(&hTimer);
	std::cout << "GPU time taken: " << GPUTime << std::endl << std::endl;

	//for (int i = 0; i < 12; ++i) {
	//	for (int j = 0; j < vmSize; ++j)
	//		std::cout << dh_output[j + vmSize * i] << " ";
	//	std::cout << "\n";
	//}

	//std::cout << "Starting CPU vector to matrix dot product: \n";
	//sdkResetTimer(&hTimer);
	//sdkStartTimer(&hTimer);
	//cpu_VectorMatrixDotProduct(h_arr1, h_arr2, h_output, vmSize);
	//sdkStopTimer(&hTimer);
	//float CPUTime = 1.0e-3 * (double)sdkGetTimerValue(&hTimer);
	//std::cout << "CPU time taken: " << CPUTime << std::endl << std::endl;

	//std::cout << "Checking result:\n";
	//bool result = true;
	//for (int i = 0; i < vmSize; ++i) {
	//	if (abs(dh_output[i] - h_output[i]) > EPSILON) {
	//		std::cout << "Result dont match at iteration " << i << "\n";
	//		std::cout << "dh_output[" << i << "] = " << dh_output[i] << std::endl;
	//		std::cout << "h_output [" << i << "] = " << h_output[i] << std::endl;
	//		std::cout << "difference: " << dh_output[i] - h_output[i] << std::endl;
	//		result = false;
	//		break;
	//	}
	//}
	//if (result) {
	//	std::cout << "Results match\n";
	//	std::cout << "Speedup: " << CPUTime / GPUTime << std::endl;
	//}

	// Clean up
	checkCudaErrors(cudaFree(d_arr1));
	checkCudaErrors(cudaFree(d_arr2));
	checkCudaErrors(cudaFree(d_output));
	delete[] h_arr1;
	delete[] h_arr2;
	delete[] h_output;
#endif
}
