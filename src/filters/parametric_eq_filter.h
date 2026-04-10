#ifndef PARAMETRIC_EQ_FILTER_H
#define PARAMETRIC_EQ_FILTER_H

#include "../pffft/pffft.h"
#include "../soloud/include/soloud.h"
#include <string>
#include <vector>

class ParametricEq;

class ParametricEqInstance : public SoLoud::FilterInstance {
  ParametricEq *mParent;
  float *mInputBuffer[MAX_CHANNELS];
  float *mMixBuffer[MAX_CHANNELS];
  float *mTemp;
  float *mFFTBuffer;
  float *mFFTWork;
  PFFFT_Setup *mFFTSetup;
  unsigned int mInputOffset[MAX_CHANNELS];
  unsigned int mMixOffset[MAX_CHANNELS];
  unsigned int mReadOffset[MAX_CHANNELS];

  // Band information precomputed for fast lookup
  int mBands;
  std::vector<float> mBandCenter;
  std::vector<float> mBandBoundary;

  // Helper functions for FFT processing
  void comp2MagPhase(float *aFFTBuffer, unsigned int aSamples);
  void magPhase2Comp(float *aFFTBuffer, unsigned int aSamples);

  // Initialize band parameters (centers, boundaries, gains)
  void initBandParameters();

  // Initialize FFT setup and allocate buffers
  void initFFTBuffers();

  // Free all allocated buffers
  void freeBuffers();

public:
  explicit ParametricEqInstance(ParametricEq *aParent);
  ~ParametricEqInstance() override;
  
  void filterChannel(float *aBuffer, unsigned int aSamples,
                     float aSamplerate, SoLoud::time aTime,
                     unsigned int aChannel, unsigned int aChannels) override;
  void fftFilterChannel(float *aFFTBuffer, unsigned int aSamples,
                        float aSamplerate, SoLoud::time aTime,
                        unsigned int aChannel, unsigned int aChannels);
  void setFilterParameter(unsigned int aAttributeId, float aValue) override;
};

class ParametricEq : public SoLoud::Filter {
public:
  // Fixed parameter indices:
  // 0: wet, 1: window size, 2: band count, 3+: per-band gains
  static constexpr int NUM_FIXED_PARAMS = 3;
  static constexpr int MAX_BANDS = 64;
  static constexpr int DEFAULT_BANDS = 3;
  static constexpr int MIN_WINDOW_SIZE = 16;
  static constexpr int MAX_WINDOW_SIZE = 65536;
  static constexpr int DEFAULT_WINDOW_SIZE = 1024;
  static constexpr float MIN_FREQ = 30.0f;
  static constexpr float MAX_FREQ = 16000.0f;
  static constexpr float SINGLE_BAND_FREQ = 1000.0f;
  
  int mBands;
  float mWet;
  std::vector<float> mGain;
  std::vector<float> mFreq;
  int mSTFT_WINDOW_SIZE;
  int mSTFT_WINDOW_HALF;
  int mSTFT_WINDOW_TWICE;
  float mFFT_SCALE;

  explicit ParametricEq(SoLoud::Soloud *aSoloud, int bands = DEFAULT_BANDS);
  
  int getParamCount() override;
  const char *getParamName(unsigned int aParamIndex) override;
  unsigned int getParamType(unsigned int aParamIndex) override;
  float getParamMax(unsigned int aParamIndex) override;
  float getParamMin(unsigned int aParamIndex) override;
  SoLoud::result setParam(unsigned int aParamIndex, float aValue);
  void setFreqs(int nBands);
  SoLoud::FilterInstance *createInstance() override;

  SoLoud::Soloud *mSoloud;
  int mChannels;
};

#endif
