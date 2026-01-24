#ifndef REVERB_H
#define REVERB_H

#include <vector>
#include <string>
#include "effect.h"

namespace upc {
  class Reverb: public upc::Effect {
    private:
      unsigned int delay_samples, buffer_len, write_idx;
      std::vector<float> buffer;     // circular delay buffer
      std::vector<float> tap_gains;  // FIR coefficients (decay^k)
	  float	mix, decay, delay_ms; // Input parameters 
      int taps; // Input parameters
    public:
      Reverb(const std::string &param = "");
	  void operator()(std::vector<float> &x);
	  void command(unsigned int);
  };
}

#endif
