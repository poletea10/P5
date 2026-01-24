#ifndef TABLE_SAMPLER
#define TABLE_SAMPLER

#include <vector>
#include <string>
#include "instrument.h"
#include "envelope_adsr.h"

namespace upc {
  class TableSampler: public upc::Instrument {
    EnvelopeADSR adsr;
    double phase = 0.0; // Phase value when synthetising over table indices
    double phaseInc = 1.0;
    double file_sr = 44100.0; // Sample rate of file
	float A;
    std::vector<float> tbl;
  public:
    TableSampler(const std::string &param = "");
    void command(long cmd, long note, long velocity=1); 
    const std::vector<float> & synthesize();
    bool is_active() const {return bActive;} 
  };
}

#endif