#ifndef SYNTH_FM
#define SYNTH_FM

#include <vector>
#include <string>
#include "instrument.h"
#include "envelope_adsr.h"
#include "effects/vibrato.h" // For the FM effect

namespace upc {
  class SynthFM: public upc::Instrument {
    EnvelopeADSR adsr;
    double phase = 0.0; // Phase value when synthetising over table indices (it can be a fraction -> in that case, we interpolate the table value)
    double phaseInc = 1.0; // Step per sample (for pitch)
	float A;
    std::vector<float> tbl;

    // Vibrato variables
    Vibrato vib;          // internal vibrato effect (created pointer since there's no )
    float I_semi;         // input I in semitones
    float N1, N2;         // input ratios
  public:
    SynthFM(const std::string &param = "");
    void command(long cmd, long note, long velocity=1); 
    const std::vector<float> & synthesize();
    bool is_active() const {return bActive;} 
  };
}

#endif