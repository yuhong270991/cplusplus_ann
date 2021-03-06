#include "neuralnet.h"
#include <cassert>
#include <iostream>
#include <string>
#include "../utils/rng.h"


//random number generator
static RNG rng;

//default activation func does nothing
template<typename T>
T identity(T x) 
{
  //std::cout << "Executing identity function" << std::endl;
  return x; 
}

template<typename T>
NeuralNet<T>::Neuron::Neuron(unsigned numOutputs, unsigned index) :
	m_output(0.f),
	m_input(0.f),
  m_gradient(1.f),
	m_index(index),
	m_func(identity),
	m_derivative(identity)
{
	for (unsigned i = 0; i < numOutputs; ++i)
	{
		m_edges.push_back(Synapse());
		m_edges.back().weight = (T)rng.random_prob();
		//m_edges.back().deltaWeight = 0.f;
	}
}

template <typename T>
void NeuralNet<T>::Neuron::setOutput(const T& val)
{
	m_output = val;
}

template <typename T>
void NeuralNet<T>::Neuron::setWeight(unsigned nextLayerNeuronIndex, const T& weight)
{
	m_edges[nextLayerNeuronIndex].weight = weight;
}

template <typename T>
void NeuralNet<T>::Neuron::setActivationFunctions(ActivationFunc f, ActivationFunc d)
{
	m_derivative = d;
	m_func = f;
}

template<typename T>
void NeuralNet<T>::Neuron::setGradient(const T & val)
{
  m_gradient = val;
}

template<typename T>
size_t NeuralNet<T>::Neuron::getNumEdges() const
{
	return m_edges.size();
}

template<typename T>
unsigned NeuralNet<T>::Neuron::getIndex() const
{
	return m_index;
}

template<typename T>
T NeuralNet<T>::Neuron::getOutput() const
{
	return m_output;
}


template<typename T>
std::vector<T> NeuralNet<T>::Neuron::getAllWeights() const
{
	std::vector<T> weights(m_edges.size(), T());
	for (size_t i = 0; i < m_edges.size(); ++i)
		weights[i] = m_edges[i].weight;
	return weights;
}

template<typename T>
T NeuralNet<T>::Neuron::getWeight(unsigned toNeuronIndex) const
{
	return m_edges[toNeuronIndex].weight;
}


template<typename T>
typename NeuralNet<T>::Neuron& NeuralNet<T>::getNeuron(unsigned layerIndex, unsigned index)
{
	assert(m_layers.size() != 0);
	return m_layers[layerIndex][index];
}

template<typename T>
NeuralNet<T>::NeuralNet(const std::vector<unsigned>& config)
{
	size_t layerCount = config.size();
	//create layers
	for (size_t l = 0; l < layerCount; ++l)
	{
		//output layer have no outputs
		unsigned numOuts = (l == layerCount - 1) ? 0 : config[l + 1];

		m_layers.push_back(Layer());

		//extra layer for bias neuron
		for (size_t n = 0; n <= config[l]; ++n)
			m_layers.back().emplace_back(numOuts, n);

		//set output of bias neuron to 1.0, nothing will change its output value
		m_layers.back().back().setOutput((T)1.f);
	}
}

template<typename T>
void NeuralNet<T>::Neuron::calculateOutput(const Layer& previousLayer)
{
  T innerproduct = (T) 0.f;

  //inner product of previous layer output and weights
  for (size_t n = 0; n < previousLayer.size(); ++n)
  {
    float tmp = previousLayer[n].getOutput();
    float tmp2 = previousLayer[n].m_edges[m_index].weight;
    innerproduct += (tmp * tmp2);
  }

  //y = f(x), where x is the innerproduct calculated previously
  m_input = innerproduct;
  assert(m_func != nullptr);
  m_output = m_func(m_input);
}

template<typename T>
T NeuralNet<T>::forward(const std::vector<T>& inputs)
{
  assert(inputs.size() == m_layers[0].size() - 1);

  //set inputs as output values of initial layer
  Layer& inputLayer = m_layers[0];
  for (size_t i = 0; i < inputs.size(); ++i)
    inputLayer[i].setOutput(inputs[i]);

  //feed forward, starting from first hidden layer
  for (size_t l = 1; l < m_layers.size(); ++l)
  {
    Layer& currentLayer = m_layers[l];
    Layer& previousLayer = m_layers[l - 1];

    //update outputs for each neuron note that bias neuron always output 1.0 so it is ignored
    for (size_t n = 0; n < currentLayer.size() - 1; ++n)
      currentLayer[n].calculateOutput(previousLayer);
  }

  //get output (assume only one neuron in output layer --> extra bias neuron ignored)
  assert(m_layers.back().size() == 2);
  T result = m_layers.back()[0].getOutput();

  return result;
}

template<typename T>
void NeuralNet<T>::back(const T & predictedVal, const T & expectedVal, float alpha)
{
  //calculate gradient at output layer
  Layer& outputLayer = m_layers.back();
  for (size_t i = 0; i < outputLayer.size() - 1; ++i)
  {
    outputLayer[i].computeOutputGradients(expectedVal);
  }

  //calculate gradients of hidden layers only
  for (size_t i = m_layers.size() - 2; i > 0; --i)
  {
    Layer& currLayer = m_layers[i];
    Layer& nextLayer = m_layers[i + 1];

    for (size_t n = 0; n < currLayer.size(); ++n)
      currLayer[n].computeHiddenGradients(nextLayer);
  }

  //update weights starting from hidden layer
  for (size_t i = m_layers.size() - 1; i > 0; --i)
  {
    Layer& currentLayer = m_layers[i];
    Layer& prevLayer = m_layers[i - 1];

    for (size_t n = 0; n < currentLayer.size() - 1; ++n)
      currentLayer[n].updateWeights(prevLayer, alpha);
  }
}


template<typename T>
void NeuralNet<T>::Neuron::computeHiddenGradients(const Layer& nextLayer)
{
  T dW = (T) 0.f;

  //sum contribution of errors at nodes of next layer
  for (size_t i = 0; i < nextLayer.size() - 1; ++i)
  {
    dW += m_edges[i].weight * nextLayer[i].m_gradient;
  }

  m_gradient = m_derivative(m_input) * dW;
}

template<typename T>
void NeuralNet<T>::Neuron::computeOutputGradients(const T & expectedValue)
{
  T delta = expectedValue - m_output;
  m_gradient = m_derivative(m_input) * delta;
}

template<typename T>
void NeuralNet<T>::Neuron::updateWeights(Layer& prevLayer, float a)
{
  //update the weights in the previous layer
  for (size_t i = 0; i < prevLayer.size(); ++i)
  {
    Neuron& prevNeuron = prevLayer[i];
    T new_deltaWeight = prevNeuron.getOutput() * m_gradient;

    //gradient descent
    T alpha = static_cast<T>(a);

    prevNeuron.m_edges[m_index].weight += alpha * new_deltaWeight;
  }
}

template<typename T>
void NeuralNet<T>::train(const Matrix& inputs, const std::vector<T>& labels, float learningRate, unsigned numIterations)
{
	//number of rows = number of inputs
	size_t numInputs = inputs.size();

	//every input should have a label
	assert(numInputs == labels.size());

  std::vector<T> finalResults(labels.size(), T());
	for (unsigned i = 0; i < numIterations; ++i)
	{
		std::vector<T> outputs(labels.size(), T());

		//feed forward and back propagate
    for (size_t j = 0; j < numInputs; ++j)
    {
      outputs[j] = forward(inputs[j]);
      back(outputs[j], labels[j], learningRate);
    }

    //error in this iteration
    T error = m_ef(outputs, labels);

		//output error
		std::cout << "Error in " << i + 1 << " iteration: " << error << std::endl;

    if(i == numIterations-1)
      finalResults = outputs;
	}

  std::cout << "--------Results in last iteration--------" << std::endl;
  std::cout << "[";
  for (size_t i = 0; i < numInputs; ++i)
  {
    std::cout << finalResults[i];
    if(i != numInputs -1)
      std::cout << ", ";
  }
  std::cout << "]" << std::endl;
}

template<typename T>
NeuralNet<T>::Layer & NeuralNet<T>::GetLayer(unsigned index)
{
	return m_layers[index];
}

template<typename T>
T NeuralNet<T>::predict(const std::vector<T>& input)
{
  assert(input.size() == m_layers[0].size()-1);
	return forward(input);
}

template<typename T>
size_t NeuralNet<T>::numLayers() const
{
	return m_layers.size();
}

template<typename T>
size_t NeuralNet<T>::numNeurons(unsigned index) const
{
	return m_layers[index].size();
}

template<typename T>
void NeuralNet<T>::setErrorFunction(ErrorFunc error_function)
{
  m_ef = error_function;
}

template<typename T>
void NeuralNet<T>::printWeights()
{
	float i1h1 = getNeuron(0, 0).getWeight(0);
	float i2h1 = getNeuron(0, 1).getWeight(0);
	float h1_bias = getNeuron(0, 2).getWeight(0);

	float i1h2 = getNeuron(0, 0).getWeight(1);
	float i2h2 = getNeuron(0, 1).getWeight(1);
	float h2_bias = getNeuron(0, 2).getWeight(1);

	float h1o1 = getNeuron(1, 0).getWeight(0);
	float h2o1 = getNeuron(1, 1).getWeight(0);
	float o1_bias = getNeuron(1, 2).getWeight(0);

	std::cout << "Input layer to hidden layer weights" << std::endl;
	std::cout << "[" << h1_bias << ", " << i1h1 << ", " << i2h1 << "] ";
	std::cout << "[" << h2_bias << ", " << i1h2 << ", " << i2h2 << "]" << std::endl;
	std::cout << "hidden layer to output layer weights" << std::endl;
	std::cout << "[" << o1_bias << ", " << h1o1 << ", " << h2o1 << "]" << std::endl;
}

