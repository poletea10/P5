// reverb.cpp  (FIR "echo/reverb" style using decaying taps)
// ---------------------------------------------------------
// This is a simple FIR reverb made of equally-spaced echoes:
//
// y[n] = (1-mix)*x[n] + mix * ( x[n] + sum_{k=1..K} (decay^k) * x[n - k*D] )
//
// where:
// - D = delay in samples (from delay_ms)
// - K = number of taps (taps)
// - decay in (0,1) controls how fast echoes die out
// - mix in [0,1] sets dry/wet balance
//
// Notes:
// - This is an FIR (no feedback), so it is always stable.
// - It sounds like a “multi-tap echo” (a very common beginner-friendly reverb-like FX).

#include <iostream>
#include <math.h>
#include <algorithm>
#include "reverb.h"
#include "keyvalue.h"

#include <stdlib.h>

using namespace upc;
using namespace std;

static float SamplingRate = 44100.0f;

Reverb::Reverb(const std::string &param) {
  KeyValue kv(param);

  // --- Parameters with defaults ---
  if (!kv.to_float("mix", mix))
    mix = 0.35f; // 0=dry, 1=fully wet

  if (!kv.to_float("decay", decay))
    decay = 0.6f; // echo decay per tap (0..1)

  if (!kv.to_float("delay_ms", delay_ms))
    delay_ms = 35.0f; // ms between taps

  if (!kv.to_int("taps", taps))
    taps = 8; // number of echoes

  // --- Clamp / sanitize ---
  mix = std::max(0.0f, std::min(1.0f, mix));
  decay = std::max(0.0f, std::min(0.999f, decay)); // keep < 1
  if (delay_ms < 1.0f) delay_ms = 1.0f;
  if (taps < 1) taps = 1;
  if (taps > 64) taps = 64; // safety cap

  // Convert delay to samples
  delay_samples = (int)roundf(delay_ms * SamplingRate / 1000.0f);
  if (delay_samples < 1) delay_samples = 1;

  // Ring buffer must hold max delay = taps * delay_samples
  buffer_len = taps * delay_samples + 1;
  buffer.assign(buffer_len, 0.0f);
  write_idx = 0;

  // Precompute tap gains: g[k] = decay^k  for k = 1..taps
  tap_gains.resize(taps + 1);
  tap_gains[0] = 1.0f; // not used as a delayed tap, but kept for clarity
  for (int k = 1; k <= taps; ++k) {
    tap_gains[k] = powf(decay, (float)k);
  }
}

void Reverb::command(unsigned int comm) {
  // Convention (like other effects): comm == 1 means "reset"
  if (comm == 1) {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    write_idx = 0;
  }
}

void Reverb::operator()(std::vector<float> &x) {
  for (unsigned int i = 0; i < x.size(); ++i) {
    float in = x[i];

    // Store current input in ring buffer
    buffer[write_idx] = in;

    // Build wet signal = sum of delayed taps (no feedback)
    float wet = 0.0f;

    // Add taps at delays k*D
    for (int k = 1; k <= taps; ++k) {
      int read_idx = write_idx - k * delay_samples;
      while (read_idx < 0) read_idx += buffer_len; // wrap
      wet += tap_gains[k] * buffer[read_idx];
    }

    // Include direct path inside the wet branch so "wet-only" still sounds coherent
    float wet_with_direct = in + wet;

    // Dry/Wet mix
    x[i] = (1.0f - mix) * in + mix * wet_with_direct;

    // Advance ring buffer write index
    write_idx++;
    if (write_idx >= buffer_len) write_idx = 0;
  }
}
