#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace ce2600 {
constexpr float pi = 3.14159265358979323846f;
inline float limit(float x, float a, float b) { return std::max(a, std::min(b, x)); }
inline float safe(float x) { return std::isfinite(x) ? limit(x, -100.f, 100.f) : 0.f; }
inline float time(float x) { return 0.001f * std::pow(10000.f, x); }
struct Trigger {
 bool high = false;
 bool process(float v) { bool old = high; if(v >= 1.f) high = true; else if(v <= .1f) high = false; return high && !old; }
};
struct Oscillator {
 float phase = 0.f, triangle = -1.f;
 static float blep(float t, float dt) {
  if(t < dt) { t /= dt; return 2*t - t*t - 1; }
  if(t > 1-dt) { t = (t-1)/dt; return t*t + 2*t + 1; }
  return 0;
 }
 struct Waves { float saw, pulse, sine, tri; };
 Waves process(float hz, float pw, float dt) {
  float step = limit(hz*dt, 1e-8f, .45f);
  phase += step; phase -= std::floor(phase);
  pw = limit(pw, std::max(.03f, step), std::min(.97f, 1-step));
  float saw = 2*phase - 1 - blep(phase, step);
  float pulse = (phase < pw ? 1.f : -1.f) + blep(phase, step) - blep(std::fmod(phase-pw+1,1), step);
  // A leaky integration of a bandlimited 50% pulse for a stable triangle.
  float square = (phase < .5f ? 1.f : -1.f) + blep(phase,step) - blep(std::fmod(phase+.5f,1),step);
  triangle = (triangle + square * 4 * step) / (1 + step * .02f);
  return {saw*5, pulse*5, std::sin(2*pi*phase)*5, limit(triangle,-1.f,1.f)*5};
 }
};
struct Envelope {
 enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } stage = IDLE;
 float value = 0; bool lastGate = false;
 float process(bool gate, bool retrigger, float a, float d, float s, float r, float dt) {
  if(gate && (!lastGate || retrigger)) stage = ATTACK;
  if(!gate && lastGate) stage = RELEASE;
  lastGate = gate;
  if(stage == ATTACK) { value += dt/std::max(a, .001f); if(value >= 1) {value=1; stage=DECAY;} }
  else if(stage == DECAY) { value -= dt/std::max(d,.001f); if(value <= s) {value=s; stage=SUSTAIN;} }
  else if(stage == SUSTAIN) value = s;
  else if(stage == RELEASE) {value -= dt/std::max(r,.001f); if(value<=0) {value=0; stage=IDLE;} }
  return value*10;
 }
};
// Four cascaded TPT one-poles with a zero-delay linear feedback solution.
// Nonlinear input saturation adds drive; this is not a transistor-level model.
struct Ladder {
 std::array<float,4> state{};
 float process(float input, float cutoff, float resonance, float sampleRate) {
  float g = std::tan(pi * limit(cutoff, 10, sampleRate*.18f) / sampleRate);
  float G = g/(1+g), k = limit(resonance,0,1)*3.95f;
  float S = 0, power=1;
  for(int i=3;i>=0;--i) { S += power*state[i]/(1+g); power *= G; }
  float u = (std::tanh(input*.2f)*5 - k*S)/(1+k*power);
  for(int i=0;i<4;++i) { float v=(u-state[i])*G; u=v+state[i]; state[i]=safe(u+v); }
  return safe(u);
 }
};
struct Noise {
 uint32_t state = 0x637E2600; float pink=0;
 float process(float color, float dt) {
  state ^= state<<13; state ^= state>>17; state ^= state<<5;
  float w = (float(state)/4294967295.f*2-1)*5;
  pink += (1-std::exp(-2*pi*650*dt))*(w-pink);
  return w*(1-color)+pink*2*color;
 }
};
// Dispersive comb/allpass network; spring-like coloration, not a physical spring model.
struct Reverb {
 struct Line {
  std::vector<float> data; size_t pos=0; float low=0;
  void setup(int n) { data.assign(std::max(2,n),0); pos=0; low=0; }
  float comb(float x, float feedback) { float y=data[pos]; low+=.22f*(y-low); data[pos]=safe(x+feedback*low); if(++pos==data.size())pos=0; return y; }
  float allpass(float x) {float b=data[pos], y=b-.5f*x; data[pos]=safe(x+.5f*y); if(++pos==data.size())pos=0; return y; }
 };
 std::array<Line,6> combs; std::array<Line,2> diffusers;
 void setup(float sr) { const float t[]={.0297f,.0371f,.0411f,.0313f,.0397f,.0437f}; for(int i=0;i<6;++i)combs[i].setup(int(sr*t[i])); diffusers[0].setup(int(sr*.0051f));diffusers[1].setup(int(sr*.0067f)); }
 std::array<float,2> process(float x) { float l=0,r=0; for(int i=0;i<3;++i){l+=combs[i].comb(x*.22f,.77f);r+=combs[i+3].comb(x*.22f,.77f);} return {diffusers[0].allpass(l),diffusers[1].allpass(r)}; }
};
}
