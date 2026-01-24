#include <iostream>
#include <math.h>
#include <sstream> // For osstring
#include "synth_fm.h"
#include "keyvalue.h"

#include <stdlib.h>

using namespace upc;
using namespace std;

SynthFM::SynthFM(const std::string &param) 
  : adsr(SamplingRate, param) {
  bActive = false;
  x.resize(BSIZE);

  /*
    You can use the class keyvalue to parse "param" and configure your instrument.
    Take a Look at keyvalue.h    
  */
  KeyValue kv(param);
  int N;

  if (!kv.to_int("N",N))
    N = 40; //default value

  // FM parameters
  if (!kv.to_float("I", I_semi)) I_semi = 1.0f;
  if (!kv.to_float("N1", N1))    N1 = 1.0f;
  if (!kv.to_float("N2", N2))    N2 = 1.0f;

  // Create a tbl with one period of a sinusoidal wave
  tbl.resize(N);
  float phase = 0, step = 2 * M_PI /(float) N;
  for (int i=0; i < N ; ++i) {
    tbl[i] = sin(phase);
    phase += step;
  } // tbl stores one period of a sinusoidal wave. We'll generate different pitches by running through the table at different speeds.
}


void SynthFM::command(long cmd, long note, long vel) {
  if (cmd == 9) {		//'Key' pressed: attack begins
    bActive = true; // Activate instrument
    adsr.start();

    double f0 = 440.0 * pow(2.0, (note - 69.0) / 12.0); // Sine freq based on note value (see Section 3.5 in pdf)

    // From Section 5
    double fc = (double)N1 * f0;
    double fm = (double)N2 * f0;

    // See Section 5, our oscillator uses fc and then the Vibrato is added
    phaseInc = fc * (double)tbl.size() / (double)SamplingRate; // Based on f0, what step should we take in the table for each sample

    // Create Vibrato effect for that note (using I and fm already calculated)
    std::ostringstream oss;
    oss << "I=" << I_semi << "; fm=" << fm << ";"; // Create vibrato parameter input string

    vib = Vibrato(oss.str());  // Create vibrato effect
    vib.command(1);            // reset buffer

	A = vel / 127.; // Amplitude of sine based on command's vel parameter
  }
  else if (cmd == 8) {	//'Key' released: sustain ends, release begins
    adsr.stop();
  }
  else if (cmd == 0) {	//Sound extinguished without waiting for release to end
    adsr.end();
  }
}


const vector<float> & SynthFM::synthesize() {
  if (not adsr.active()) {
    x.assign(x.size(), 0);
    bActive = false;
    return x;
  }
  else if (not bActive)
    return x;

  for (unsigned int i=0; i<x.size(); ++i) {

    // Check phase floor (the tbl sample index just before our phase) and phase fraction (how much over floor index are we)
    int i0 = (int)floor(phase); // Closest index below phase
    double frac = phase - i0; // How much over lower index are we
    int i1 = i0+1; // Closest index above phase

    if (i1 >= (int)tbl.size()) i1 = 0; // In case we're at the end of the table

    // Linear interpolation of table values for our phase
    double s = (1.0 - frac) * tbl[i0] + frac * tbl[i1];

    x[i] = (float)(A * s); // Generate sample
    phase += phaseInc; // Go forward phaseInc (set to match expected pitch)

    if (phase >= tbl.size())
      phase = phase-tbl.size();
  }
  vib(x); // Apply vibrato effect
  adsr(x); //apply envelope to x and update internal status of ADSR

  return x;
}