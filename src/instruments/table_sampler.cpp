#include <iostream>
#include <math.h>
#include "table_sampler.h"
#include "keyvalue.h"
#include "wavfile_mono.h" // To read wav file

#include <stdlib.h>

using namespace upc;
using namespace std;

TableSampler::TableSampler(const std::string &param) 
  : adsr(SamplingRate, param) {
  bActive = false;
  x.resize(BSIZE);

  /*
    You can use the class keyvalue to parse "param" and configure your instrument.
    Take a Look at keyvalue.h    
  */
  KeyValue kv(param);

  // Read WAV file
  std::string file_name;
  static std::string kv_null;

  file_name = kv("file");
  if (file_name == kv_null) {
    cerr << "Error: missing 'file' parameter for TableSampler (e.g. file=kick.wav;)" << endl;
    throw -1;
  }

  unsigned int fm = 0;
  if (readwav_mono(file_name, fm, tbl) < 0) { // Saves wav file to tbl
    cerr << "Error: cannot read wav file '" << file_name << "' for TableSampler" << endl;
    throw -1;
  }

  // Store the file sample rate
  file_sr = (double)fm;
  
  // Start playback at beginning
  phase = 0.0;
  phaseInc = 1.0;

  // Optional: if file sample rate != engine sample rate, compensate
  // so that "phaseInc = 1" plays at correct speed.
  if (file_sr > 0.0) {
    phaseInc *= (file_sr / (double)SamplingRate);
  }

  A = 0.0f;
}


void TableSampler::command(long cmd, long note, long vel) {
  if (cmd == 9) {		//'Key' pressed: attack begins
    bActive = true; // Activate instrument
    adsr.start();

    phase = 0;
    
    // We will make the sampler always play the wav file without pitch change (doesn't matter the key pressed)

	A = vel / 127.; // Amplitude of sine based on command's vel parameter
  }
  else if (cmd == 8) {	//'Key' released: sustain ends, release begins
    adsr.stop();
  }
  else if (cmd == 0) {	//Sound extinguished without waiting for release to end
    adsr.end();
  }
}


const vector<float> & TableSampler::synthesize() {
  if (!adsr.active()) {
    x.assign(x.size(), 0.0f);
    bActive = false;
    return x;
  }
  else if (!bActive) {
    return x;
  }

  const int N = (int)tbl.size();

  for (unsigned int i = 0; i < x.size(); ++i) {

    // If we've reached the end of the sample, output silence and finish the note.
    if ((int)floor(phase) >= N) {
      x[i] = 0.0f;

      // End the envelope and mark inactive after this block
      // (This prevents a drum hit from looping forever)
      adsr.end();
      bActive = false;

      // Fill remainder with zeros
      for (unsigned int j = i + 1; j < x.size(); ++j) x[j] = 0.0f;
      break;
    }

    // Linear interpolation (fractional index)
    int i0 = (int)floor(phase);
    double frac = phase - i0;
    int i1 = i0 + 1;

    // Clamp i1 at end (no wrap!)
    if (i1 >= N) i1 = N - 1;

    double s = (1.0 - frac) * tbl[i0] + frac * tbl[i1];

    x[i] = (float)(A * s);

    phase += phaseInc;
  }

  adsr(x);
  return x;
}