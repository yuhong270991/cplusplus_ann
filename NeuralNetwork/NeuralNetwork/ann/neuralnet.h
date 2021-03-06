#ifndef _NEURAL_NET_H
#define _NEURAL_NET_H
#pragma once

#include <vector>

template <typename T>
class NeuralNet
{
public:
  struct Synapse
  {
    T weight;
    //T deltaWeight;
  };

  class Neuron;
  using ActivationFunc = T(*)(T);
  using ErrorFunc = T (*)(const std::vector<T>&, const std::vector<T>&);
  using Layer = std::vector<Neuron>;
  using Matrix = std::vector<std::vector<T>>;

  //currently this network does not allow user to specify input-output configuration,
  //assumes output layer has only one neuron 
  class Neuron
  {
  public:
    Neuron(unsigned numOutputs, unsigned index);
    void calculateOutput(const Layer& previousLayer);
    void computeHiddenGradients(const Layer& nextLayer);
    void computeOutputGradients(const T& expectedValue);
    void updateWeights(Layer& previousLayer, float a);
    void setGradient(const T& val);
    void setWeight(unsigned nextLayerNeuronIndex, const T& weight);
    void setOutput(const T& val);
    void setActivationFunctions(ActivationFunc func, ActivationFunc derivative);
	  
    unsigned getIndex()const;
    T getWeight(unsigned toNeuronIndex) const;
    T getOutput() const;
    size_t getNumEdges() const;
    std::vector<T> getAllWeights() const;

  private:
    ActivationFunc m_func;
    ActivationFunc m_derivative;
    T m_output;
    T m_input;
    T m_gradient;
    unsigned m_index;
    std::vector<T> m_batchedOutputs;
    std::vector<Synapse> m_edges;
  };

  NeuralNet(const std::vector<unsigned>& config);
  //predicts output after training
  T predict(const std::vector<T>& input);
  //one forward and one back for every sample
  void train(const Matrix& inputs, const std::vector<T>& labels, float learningRate, unsigned numIter);

  Neuron& getNeuron(unsigned layerIndex, unsigned index);
  Layer& GetLayer(unsigned index);
  size_t numLayers() const;
  size_t numNeurons(unsigned layerIndex) const;
  void printWeights();
  void setErrorFunction(ErrorFunc error_function);

private:
  T forward(const std::vector<T>& inputvalues);
  void back(const T& predictedVal, const T& expectedVal, float alpha);

  ErrorFunc m_ef;
  std::vector<Layer> m_layers;
};

#include "neuralnet.cpp"
#endif
