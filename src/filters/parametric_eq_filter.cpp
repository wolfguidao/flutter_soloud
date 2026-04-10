#include "parametric_eq_filter.h"
#include "soloud.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

// MSVC fix: Undefine min/max macros that may be defined by Windows.h
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

ParametricEqInstance::ParametricEqInstance(ParametricEq *aParent) 
  : mParent(aParent)
  , mTemp(nullptr)
  , mFFTBuffer(nullptr)
  , mFFTWork(nullptr)
  , mFFTSetup(nullptr)
  , mBands(0) 
{
  if (mParent == nullptr) {
    return;
  }

  // Initialize buffer pointers to null
  for (int i = 0; i < MAX_CHANNELS; i++) {
    mInputBuffer[i] = nullptr;
    mMixBuffer[i] = nullptr;
    mInputOffset[i] = 0;
    mMixOffset[i] = 0;
    mReadOffset[i] = 0;
  }

  // Initialize FFT buffers (must happen before initBandParameters)
  initFFTBuffers();
  
  // Initialize band parameters and set up params
  initBandParameters();
}

void ParametricEqInstance::comp2MagPhase(float *aFFTBuffer,
                                         unsigned int aSamples) {
  for (unsigned int i = 0; i < aSamples; i++) {
    float re = aFFTBuffer[i * 2];
    float im = aFFTBuffer[i * 2 + 1];
    aFFTBuffer[i * 2] = std::sqrt(re * re + im * im);
    aFFTBuffer[i * 2 + 1] = std::atan2(im, re);
  }
}

void ParametricEqInstance::magPhase2Comp(float *aFFTBuffer,
                                         unsigned int aSamples) {
  for (unsigned int i = 0; i < aSamples; i++) {
    float mag = aFFTBuffer[i * 2];
    float phase = aFFTBuffer[i * 2 + 1];
    aFFTBuffer[i * 2] = mag * std::cos(phase);
    aFFTBuffer[i * 2 + 1] = mag * std::sin(phase);
  }
}

void ParametricEqInstance::initBandParameters() {
  if (mParent == nullptr) {
    return;
  }

  // Copy band count from parent
  mBands = mParent->mBands;

  // Initialize parameters
  initParams(ParametricEq::NUM_FIXED_PARAMS + mBands);

  // Copy parent values into parameter slots
  mParam[0] = mParent->mWet;
  mParam[1] = static_cast<float>(mParent->mSTFT_WINDOW_SIZE);
  mParam[2] = static_cast<float>(mBands);

  // Copy band gains into parameter slots (params[3..3+bands-1])
  for (int i = 0; i < mBands; i++) {
    mParam[ParametricEq::NUM_FIXED_PARAMS + i] = mParent->mGain[i];
  }

  // Precompute band centers and boundaries
  mBandCenter.resize(mBands);
  mBandBoundary.resize(mBands + 1);
  
  for (int i = 0; i < mBands; i++) {
    mBandCenter[i] = mParent->mFreq[i];
  }

  // Boundaries: first = 0, last = effectively infinity (clamped to Nyquist when used)
  mBandBoundary[0] = 0.0f;
  for (int i = 0; i < mBands - 1; i++) {
    mBandBoundary[i + 1] = 0.5f * (mBandCenter[i] + mBandCenter[i + 1]);
  }
  mBandBoundary[mBands] = 1e9f;
}

void ParametricEqInstance::initFFTBuffers() {
  if (mParent == nullptr) {
    return;
  }

  // Validate window size
  int windowSize = mParent->mSTFT_WINDOW_SIZE;
  if (windowSize < ParametricEq::MIN_WINDOW_SIZE || 
      windowSize > ParametricEq::MAX_WINDOW_SIZE) {
    return;
  }

  // Free existing resources first
  freeBuffers();

  // Initialize FFT setup
  mFFTSetup = pffft_new_setup(windowSize, PFFFT_COMPLEX);
  if (mFFTSetup == nullptr) {
    return;
  }

  // Allocate aligned buffers
  int windowTwice = mParent->mSTFT_WINDOW_TWICE;
  mFFTBuffer = static_cast<float*>(pffft_aligned_malloc(windowTwice * sizeof(float)));
  mFFTWork = static_cast<float*>(pffft_aligned_malloc(windowTwice * sizeof(float)));
  mTemp = static_cast<float*>(pffft_aligned_malloc(windowTwice * sizeof(float)));

  // Check allocation success
  if (mFFTBuffer == nullptr || mFFTWork == nullptr || mTemp == nullptr) {
    freeBuffers();
    return;
  }

  // Reset channel buffer offsets
  for (int i = 0; i < MAX_CHANNELS; i++) {
    mInputOffset[i] = windowSize;
    mMixOffset[i] = mParent->mSTFT_WINDOW_HALF;
    mReadOffset[i] = 0;
  }
}

void ParametricEqInstance::freeBuffers() {
  if (mFFTSetup != nullptr) {
    pffft_destroy_setup(mFFTSetup);
    mFFTSetup = nullptr;
  }
  if (mFFTBuffer != nullptr) {
    pffft_aligned_free(mFFTBuffer);
    mFFTBuffer = nullptr;
  }
  if (mFFTWork != nullptr) {
    pffft_aligned_free(mFFTWork);
    mFFTWork = nullptr;
  }
  if (mTemp != nullptr) {
    pffft_aligned_free(mTemp);
    mTemp = nullptr;
  }

  for (int i = 0; i < MAX_CHANNELS; i++) {
    delete[] mInputBuffer[i];
    delete[] mMixBuffer[i];
    mInputBuffer[i] = nullptr;
    mMixBuffer[i] = nullptr;
  }
}

ParametricEqInstance::~ParametricEqInstance() {
  freeBuffers();
}

void ParametricEqInstance::setFilterParameter(unsigned int aAttributeId,
                                              float aValue) {
  if (aAttributeId >= mNumParams || mParent == nullptr) {
    return;
  }

  // Disable any active fader for this parameter
  mParamFader[aAttributeId].mActive = 0;

  switch (aAttributeId) {
  case 0: // wet
    if (mParent->mWet != aValue) {
      mParam[0] = aValue;
      mParent->mWet = aValue;
    }
    break;
    
  case 1: { // window size
    int newSize = static_cast<int>(aValue);
    if (mParent->mSTFT_WINDOW_SIZE != newSize) {
      mParent->mSTFT_WINDOW_SIZE = newSize;
      mParent->mSTFT_WINDOW_HALF = newSize >> 1;
      mParent->mSTFT_WINDOW_TWICE = newSize << 1;
      mParent->mFFT_SCALE = 1.0f / static_cast<float>(newSize);
      initFFTBuffers();
    }
    break;
  }
  
  case 2: { // number of bands
    int newBands = static_cast<int>(aValue);
    if (mParent->mBands != newBands) {
      // Update parent configuration first
      mParent->setFreqs(newBands);
      
      // Reallocate params with new size
      initParams(ParametricEq::NUM_FIXED_PARAMS + newBands);
      
      // Restore fixed parameters
      mParam[0] = mParent->mWet;
      mParam[1] = static_cast<float>(mParent->mSTFT_WINDOW_SIZE);
      mParam[2] = static_cast<float>(newBands);
      
      // Reinitialize band parameters
      initBandParameters();
    }
    break;
  }
  
  default: // band gains (3+)
    mParam[aAttributeId] = aValue;
    break;
  }
}

void ParametricEqInstance::filterChannel(float *aBuffer, unsigned int aSamples,
                                         float aSamplerate, SoLoud::time aTime,
                                         unsigned int aChannel,
                                         unsigned int aChannels) {
  // Safety checks
  if (mParent == nullptr || aBuffer == nullptr) {
    return;
  }
  if (aChannel >= MAX_CHANNELS) {
    return;
  }
  if (mFFTSetup == nullptr || mFFTBuffer == nullptr || mTemp == nullptr) {
    return;
  }

  int windowSize = mParent->mSTFT_WINDOW_SIZE;
  int windowHalf = mParent->mSTFT_WINDOW_HALF;
  int windowTwice = mParent->mSTFT_WINDOW_TWICE;
  float fftScale = mParent->mFFT_SCALE;

  // Lazy initialization of channel buffers
  if (mInputBuffer[aChannel] == nullptr) {
    mInputBuffer[aChannel] = new float[windowTwice]();
    mMixBuffer[aChannel] = new float[windowTwice]();
  }

  unsigned int ofs = 0;
  unsigned int inputofs = mInputOffset[aChannel];
  unsigned int mixofs = mMixOffset[aChannel];
  unsigned int readofs = mReadOffset[aChannel];

  while (ofs < aSamples) {
    int samples = windowHalf - (inputofs & (windowHalf - 1));
    if (ofs + static_cast<unsigned int>(samples) > aSamples) {
      samples = static_cast<int>(aSamples - ofs);
    }

    // Copy input samples
    for (int i = 0; i < samples; i++) {
      unsigned int idx = (inputofs + windowHalf) & (windowTwice - 1);
      mInputBuffer[aChannel][idx] = aBuffer[ofs + i];
      mMixBuffer[aChannel][idx] = 0.0f;
      inputofs++;
    }

    // Process when we have a full hop
    if ((inputofs & (windowHalf - 1)) == 0) {
      // Copy to FFT buffer
      for (int i = 0; i < windowSize; i++) {
        unsigned int srcIdx = (inputofs + windowTwice - windowHalf + i) & (windowTwice - 1);
        mFFTBuffer[i * 2] = mInputBuffer[aChannel][srcIdx];
        mFFTBuffer[i * 2 + 1] = 0.0f;
      }

      // Forward FFT
      pffft_transform_ordered(mFFTSetup, mFFTBuffer, mTemp, mFFTWork, PFFFT_FORWARD);

      // Apply EQ
      fftFilterChannel(mTemp, windowSize, aSamplerate, aTime, aChannel, aChannels);

      // Inverse FFT
      pffft_transform_ordered(mFFTSetup, mTemp, mFFTBuffer, mFFTWork, PFFFT_BACKWARD);

      // Apply window and overlap-add
      for (int i = 0; i < windowSize; i++) {
        float window = 0.5f * (1.0f - std::cos((2.0f * M_PI * i) / windowSize));
        float sample = mFFTBuffer[i * 2] * fftScale * window;
        mMixBuffer[aChannel][mixofs & (windowTwice - 1)] += sample;
        mixofs++;
      }
      mixofs -= windowHalf;
    }

    // Output processed samples
    for (int i = 0; i < samples; i++) {
      aBuffer[ofs + i] = mMixBuffer[aChannel][readofs & (windowTwice - 1)];
      readofs++;
    }

    ofs += samples;
  }

  mInputOffset[aChannel] = inputofs;
  mReadOffset[aChannel] = readofs;
  mMixOffset[aChannel] = mixofs;
}

void ParametricEqInstance::fftFilterChannel(float *aFFTBuffer,
                                            unsigned int aSamples,
                                            float aSamplerate,
                                            SoLoud::time /*aTime*/,
                                            unsigned int /*aChannel*/,
                                            unsigned int /*aChannels*/) {
  comp2MagPhase(aFFTBuffer, aSamples);
  
  float nyquist = aSamplerate * 0.5f;
  unsigned int halfSamples = aSamples / 2;

  for (unsigned int i = 0; i < aSamples; i++) {
    // Map negative frequency bins to positive
    unsigned int freqBin = (i <= halfSamples) ? i : (aSamples - i);
    float currentFreq = static_cast<float>(freqBin) * aSamplerate / static_cast<float>(aSamples);

    float gain = 0.0f;
    float weightSum = 0.0f;

    for (int b = 0; b < mBands; b++) {
      float center = mBandCenter[b];
      float low = mBandBoundary[b];
      float high = mBandBoundary[b + 1];
      
      if (high > nyquist) {
        high = nyquist;
      }

      float leftHalfWidth = center - low;
      float rightHalfWidth = high - center;
      float weight = 0.0f;

      if (currentFreq <= center && leftHalfWidth > 0.0f) {
        float d = center - currentFreq;
        float w = 1.0f - (d / leftHalfWidth);
        if (w >= 0.0f) {
          weight = std::max(w, 0.001f);
        }
      } else if (currentFreq > center && rightHalfWidth > 0.0f) {
        float d = currentFreq - center;
        float w = 1.0f - (d / rightHalfWidth);
        if (w >= 0.0f) {
          weight = std::max(w, 0.001f);
        }
      } else if (leftHalfWidth <= 0.0f && rightHalfWidth <= 0.0f) {
        weight = (std::fabs(currentFreq - center) < 1e-6f) ? 1.0f : 0.0f;
      }

      float bandGain = mParam[ParametricEq::NUM_FIXED_PARAMS + b];
      gain += bandGain * weight;
      weightSum += weight;
    }

    if (weightSum > 0.0f) {
      gain /= weightSum;
    } else {
      gain = 1.0f;
    }

    aFFTBuffer[i * 2] *= gain;
  }

  magPhase2Comp(aFFTBuffer, aSamples);
}

SoLoud::result ParametricEq::setParam(unsigned int aParamIndex, float aValue) {
  (void)aParamIndex;
  (void)aValue;
  // Not used - parameters are set via FilterInstance
  return SoLoud::SO_NO_ERROR;
}

int ParametricEq::getParamCount() {
  return NUM_FIXED_PARAMS + mBands;
}

const char *ParametricEq::getParamName(unsigned int aParamIndex) {
  switch (aParamIndex) {
  case 0:
    return "Wet";
  case 1:
    return "Window Size";
  case 2:
    return "Bands Count";
  default:
    // Use 0-based band numbering (band 0, band 1, etc.)
    static thread_local char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Band %d Gain", 
                  aParamIndex - NUM_FIXED_PARAMS);
    return buffer;
  }
}

unsigned int ParametricEq::getParamType(unsigned int aParamIndex) {
  if (aParamIndex == 1 || aParamIndex == 2) {
    return INT_PARAM;
  }
  return FLOAT_PARAM;
}

float ParametricEq::getParamMax(unsigned int aParamIndex) {
  switch (aParamIndex) {
  case 0:
    return 1.0f;  // wet
  case 1:
    return static_cast<float>(MAX_WINDOW_SIZE);  // window size
  case 2:
    return static_cast<float>(MAX_BANDS);  // band count
  default:
    return 4.0f;  // band gains
  }
}

float ParametricEq::getParamMin(unsigned int aParamIndex) {
  switch (aParamIndex) {
  case 0:
    return 0.0f;  // wet
  case 1:
    return static_cast<float>(MIN_WINDOW_SIZE);  // window size
  case 2:
    return 1.0f;  // band count
  default:
    return 0.0f;  // band gains
  }
}

void ParametricEq::setFreqs(int nBands) {
  // Clamp band count
  mBands = std::max(1, std::min(nBands, MAX_BANDS));

  // Resize vectors
  mGain.assign(mBands, 1.0f);
  mFreq.resize(mBands);

  // Geometric spacing between MIN_FREQ and MAX_FREQ
  if (mBands == 1) {
    mFreq[0] = SINGLE_BAND_FREQ;
  } else {
    float ratio = MAX_FREQ / MIN_FREQ;
    for (int i = 0; i < mBands; i++) {
      float t = static_cast<float>(i) / static_cast<float>(mBands - 1);
      mFreq[i] = MIN_FREQ * std::pow(ratio, t);
    }
  }
}

ParametricEq::ParametricEq(SoLoud::Soloud *aSoloud, int bands) 
  : mBands(0)
  , mWet(1.0f)
  , mSTFT_WINDOW_SIZE(DEFAULT_WINDOW_SIZE)
  , mSTFT_WINDOW_HALF(DEFAULT_WINDOW_SIZE >> 1)
  , mSTFT_WINDOW_TWICE(DEFAULT_WINDOW_SIZE << 1)
  , mFFT_SCALE(1.0f / static_cast<float>(DEFAULT_WINDOW_SIZE))
  , mSoloud(aSoloud)
  , mChannels(aSoloud ? aSoloud->mChannels : 2)
{
  setFreqs(bands);
}

SoLoud::FilterInstance *ParametricEq::createInstance() {
  return new ParametricEqInstance(this);
}
