#include "../src/CE2600.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
static void check(bool condition,const char* message){if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static void connect(CE2600& m,int id,float voltage){m.inputs[id].channels=1;m.inputs[id].setVoltage(voltage);}
static void defaults(CE2600& m){for(size_t i=0;i<m.params.size();++i)if(m.paramQuantities[i])m.params[i].setValue(m.paramQuantities[i]->getDefaultValue());}
static void wav(const char* path,const std::vector<float>& samples,int sr){
 std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t v){f.write((char*)&v,2);};auto u32=[&](uint32_t v){f.write((char*)&v,4);};
 f.write("RIFF",4);u32(36+samples.size()*2);f.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(sr);u32(sr*2);u16(2);u16(16);f.write("data",4);u32(samples.size()*2);
 for(float x:samples){int16_t v=int16_t(ce2600::limit(x/10,-1,1)*32767);f.write((char*)&v,2);}
}
int main(){
 using namespace ce2600;
 for(float sr:{44100.f,48000.f,96000.f}){
  float dt=1/sr;
  Oscillator o;float last=0,sum=0;int crossings=0;
  for(int t=0;t<int(sr);++t){auto w=o.process(440,.5f,dt);if(last<0&&w.sine>=0)++crossings;last=w.sine;sum+=w.tri;check(std::isfinite(w.saw)&&std::fabs(w.pulse)<11,"oscillator finite / bounded");}
  check(std::abs(crossings-440)<=1,"oscillator tuned at 440 Hz");check(std::fabs(sum/sr)<.15f,"triangle centered");
  Envelope e;for(int t=0;t<int(sr*.31f);++t)e.process(true,false,.01,.1,.4,.05,dt);
  check(std::fabs(e.value-.4f)<.001,"ADSR sustains");for(int t=0;t<int(sr*.06f);++t)e.process(false,false,.01,.1,.4,.05,dt);check(e.value==0,"ADSR releases to zero");
  e.process(false,true,.01,.1,.4,.05,dt);check(e.value==0,"retrigger without held gate does not latch sustain");
  Ladder low,high;double lo=0,hi=0;
  for(int t=0;t<int(sr);++t){float a=low.process(std::sin(2*pi*100*t/sr)*5,1000,.1,sr);float b=high.process(std::sin(2*pi*10000*t/sr)*5,1000,.1,sr);lo+=a*a;hi+=b*b;}
  check(lo>hi*100,"ladder attenuates high frequencies");
  Ladder extreme;for(int t=0;t<int(sr);++t){float x=extreme.process(t%2?100:-100,std::pow(2.f,float(t%17)),1,sr);check(std::isfinite(x)&&std::fabs(x)<100,"resonant filter stable under cutoff sweeps");}
  CE2600 m;defaults(m);engine::Module::ProcessArgs args;args.sampleRate=sr;args.sampleTime=dt;
  engine::Module::SampleRateChangeEvent change;change.sampleRate=sr;change.sampleTime=dt;m.onSampleRateChange(change);
  double silence=0,sound=0;int count=0;float prev=0;
  for(int t=0;t<int(sr);++t){m.process(args);silence+=std::fabs(m.outputs[CE2600::VCA_OUT].getVoltage());}
  check(silence==0,"default VCA is silent before gate");connect(m,CE2600::GATE,10);
  for(int t=0;t<int(sr);++t){m.process(args);float v=m.outputs[CE2600::VCA_OUT].getVoltage();sound+=v*v;float sine=m.outputs[CE2600::OSC_OUT+2].getVoltage();if(prev<0&&sine>=0)++count;prev=sine;for(auto& out:m.outputs)check(std::isfinite(out.getVoltage()),"all module outputs finite");}
  check(sound/sr>.01,"normalled voice sounds with gate");check(std::abs(count-262)<=2,"default oscillator follows Rack C4 standard");
  connect(m,CE2600::PITCH,1);count=0;
  for(int t=0;t<int(sr);++t){m.process(args);float v=m.outputs[CE2600::OSC_OUT+2].getVoltage();if(prev<0&&v>=0)++count;prev=v;}
  check(std::abs(count-523)<=2,"one volt doubles oscillator frequency");
  connect(m,CE2600::VCA_IN,0);for(int t=0;t<100;++t)m.process(args);check(m.outputs[CE2600::VCA_OUT].getVoltage()==0,"patching VCA audio breaks filter normalization");
  connect(m,CE2600::SH_IN,3);connect(m,CE2600::CLOCK_IN,0);m.process(args);connect(m,CE2600::CLOCK_IN,10);m.process(args);check(m.outputs[CE2600::SH_OUT].getVoltage()==3,"S&H captures external input on rising edge");
  connect(m,CE2600::SH_IN,-2);for(int t=0;t<100;++t)m.process(args);check(m.outputs[CE2600::SH_OUT].getVoltage()==3,"S&H holds between edges");
  connect(m,CE2600::CLOCK_IN,0);m.process(args);connect(m,CE2600::CLOCK_IN,10);m.process(args);check(m.outputs[CE2600::SH_OUT].getVoltage()==-2,"S&H samples next edge");
  Reverb rv;rv.setup(sr);double energy=0,tail=0,stereo=0;
  for(int t=0;t<int(sr*4);++t){auto y=rv.process(t==0?5:0);energy+=y[0]*y[0];if(t>sr*3)tail+=y[0]*y[0];stereo+=std::fabs(y[0]-y[1]);check(std::isfinite(y[0])&&std::fabs(y[0])<10,"reverb finite / bounded");}
  check(energy>.01&&tail<energy*.001&&stereo>.01,"stereo reverb decays");
  std::printf("PASS %.0f Hz: tuning, envelopes, filter, normalled voice, patch override, S&H, reverb\n",sr);
 }
 // Exercise the actual three-module transport and DSP callbacks.
 {
  CE2600OrangeTop top; CE2600OrangeMiddle middle; CE2600OrangeBottom bottom;
  defaults(top); defaults(middle); defaults(bottom);
  bottom.params[CE2600::PHONES_LEVEL].setValue(1);
  engine::Module::ProcessArgs pa;pa.sampleRate=44100;pa.sampleTime=1/44100.f;
  connect(middle,CE2600::GATE,10);connect(bottom,CE2600::PITCH,0);
  auto tick=[&](){bottom.process(pa);middle.process(pa);top.process(pa);};
  double energy=0;
  for(int t=0;t<44100;++t){tick();float y=top.outputs[CE2600::LEFT_OUT].getVoltage();energy+=y*y;}
  check(top.linked.load()&&middle.linked.load()&&bottom.linked.load(),"all three panels linked");
  check(energy/44100>.01,"middle gate controls top VCA");
  // Stereo phones, power standby and the envelope gate selector.
  bottom.outputs[CE2600::PHONES_OUT].channels=1;tick();check(bottom.outputs[CE2600::PHONES_OUT].getChannels()==2&&bottom.outputs[CE2600::PHONES_OUT].getVoltage(0)==bottom.outputs[CE2600::LEFT_OUT].getVoltage()&&bottom.outputs[CE2600::LEFT_OUT].getVoltage()!=0&&bottom.outputs[CE2600::PHONES_OUT].getVoltage(1)==bottom.outputs[CE2600::RIGHT_OUT].getVoltage(),"bottom phones carry left and right");
  bottom.params[CE2600::POWER].setValue(0);double standby=0;for(int t=0;t<1000;++t){tick();standby+=std::fabs(top.outputs[CE2600::LEFT_OUT].getVoltage());}
  check(standby==0&&bottom.lights[CE2600::STANDBY_LIGHT].getBrightness()==1,"standby silences outputs and lights STANDBY");bottom.params[CE2600::POWER].setValue(1);
  middle.inputs[CE2600::GATE].channels=0;for(int t=0;t<44100;++t)tick();check(top.outputs[CE2600::ADSR_OUT].getVoltage()<.01f,"ADSR idle without keyboard gate");
  middle.params[CE2600::GATE_SELECT].setValue(1);float peak=0;for(int t=0;t<44100;++t){tick();peak=std::max(peak,top.outputs[CE2600::ADSR_OUT].getVoltage());}
  check(peak>5,"gate switch down runs envelopes from the S/H clock");middle.params[CE2600::GATE_SELECT].setValue(0);connect(middle,CE2600::GATE,10);
  connect(bottom,CE2600::PITCH,1);float last=0;int cycles=0;
  for(int t=0;t<44100;++t){tick();float y=top.outputs[CE2600::OSC_OUT+2].getVoltage();if(last<0&&y>=0)++cycles;last=y;}
  check(std::abs(cycles-523)<2,"bottom keyboard CV controls upper oscillators");
  connect(bottom,CE2600::SH_IN,3);connect(bottom,CE2600::CLOCK_IN,0);tick();connect(bottom,CE2600::CLOCK_IN,10);tick();tick();
  check(bottom.outputs[CE2600::SH_OUT].getVoltage()==3,"bottom outputs return upper DSP result");
  connect(middle,CE2600::VCA_IN,0);tick();check(top.outputs[CE2600::VCA_OUT].getVoltage()==0,"middle patch cable breaks normalled VCF connection");
  middle.inputs[CE2600::VCA_IN].channels=0;middle.release();top.process(pa);check(!top.linked.load(),"removed middle detected");middle.process(pa);top.process(pa);check(top.linked.load(),"middle reconnects");
  CE2600OrangeTop duplicate;defaults(duplicate);duplicate.process(pa);check(duplicate.conflict.load(),"duplicate panel cannot hijack instrument");
  duplicate.group.store(1);duplicate.process(pa);check(!duplicate.conflict.load(),"second instrument link independent");
  json_t* saved=duplicate.dataToJson();CE2600OrangeBottom restored;restored.dataFromJson(saved);json_decref(saved);check(restored.group.load()==1,"instrument link survives serialization");
  // Pausing the engine owner clears remote outputs rather than holding DC.
  top.processBypass(pa);bottom.process(pa);check(bottom.outputs[CE2600::SH_OUT].getVoltage()==0,"bypassed top silences remote outputs");
  top.process(pa);check(!top.conflict.load(),"top can resume after bypass");
  std::puts("PASS three-panel linking, pitch/gate, phones, standby, gate selector, patch override, removal, conflicts, group isolation, save/restore and bypass");
 }
 {
  Context first,second;contextSet(&first);CE2600OrangeTop a;defaults(a);contextSet(&second);CE2600OrangeTop b;defaults(b);
  engine::Module::ProcessArgs pa;pa.sampleRate=44100;pa.sampleTime=1/44100.f;
  a.process(pa);b.process(pa);check(!a.conflict.load()&&!b.conflict.load(),"separate Rack instances never share buses");contextSet(nullptr);
  std::puts("PASS separate Rack context isolation");
 }
 {
  CE2600OrangeTop top; CE2600OrangeMiddle middle; CE2600OrangeBottom bottom;
  defaults(top); defaults(middle); defaults(bottom);
  engine::Module::ProcessArgs args;args.sampleRate=48000;args.sampleTime=1.f/48000;
  auto step=[&](){bottom.process(args);middle.process(args);top.process(args);};
  connect(middle,CE2600::GATE,10);
  connect(bottom,CE2600::PITCH,-1);
  connect(bottom,CE2600::UPPER_CV,1);
  bottom.params[CE2600::DUO].setValue(1);
  for(int i=0;i<512;++i)step();
  check(top.linked.load()&&!top.conflict.load(),"Orange panels link");
  check(std::fabs(bottom.outputs[CE2600::UPPER_OUT].getVoltage()-1)<.001,"Orange duo upper voice pitch");
  check(bottom.outputs[CE2600::KEY_GATE_OUT].getVoltage()==10,"Orange keyboard gate output");
  check(std::fabs(bottom.outputs[CE2600::LFO_OUT].getVoltage())>1,"Orange LFO reaches bottom output");
  check(std::fabs(top.outputs[CE2600::MIX_OUT].getVoltage())>.001,"Orange mixer tap carries signal");
  for(int i=0;i<3;++i)for(int k=3;k<5;++k){auto* q=top.paramQuantities[CE2600::OSC+i*7+k];check(q->getMinValue()==0&&q->getDefaultValue()==0&&q->getMaxValue()==1,"FM and PWM sliders run from no modulation at the bottom");}
  {float before=top.osc[1].phase;connect(middle,CE2600::OSC_IN+5,5);step();float still=top.osc[1].phase-before;
   middle.params[CE2600::OSC+10].setValue(1);step();step();float driven=top.osc[1].phase;step();driven=top.osc[1].phase-driven;
   check(std::fabs(driven-still)>1e-5f,"Raising VCO 2 FM slider adds modulation");middle.params[CE2600::OSC+10].setValue(0);middle.inputs[CE2600::OSC_IN+5].channels=0;}
  check(top.outputs[CE2600::OUT_COPY].getVoltage()==top.outputs[CE2600::LEFT_OUT].getVoltage() && top.outputs[CE2600::OUT_COPY+1].getVoltage()==top.outputs[CE2600::RIGHT_OUT].getVoltage(),"Orange 3.5 mm outputs copy the main outputs");
  float lfoLow=10,lfoHigh=-10;for(int i=0;i<48000;++i){step();float v=top.outputs[CE2600::LFO_OUT].getVoltage();lfoLow=std::min(lfoLow,v);lfoHigh=std::max(lfoHigh,v);}
  check(lfoLow<-4.9f && lfoHigh>4.9f,"Orange LFO triangle spans +/-5 V");
  check((bottom.lights[CE2600::SWITCH_A_LIGHT].getBrightness()>.5f)!=(bottom.lights[CE2600::SWITCH_B_LIGHT].getBrightness()>.5f),"Orange switch lights show one side on the bottom panel");
  bottom.params[CE2600::PHONES_LEVEL].setValue(0);
  for(int i=0;i<8;++i)step();
  check(bottom.outputs[CE2600::PHONES_OUT].getVoltage(0)==0 && bottom.outputs[CE2600::PHONES_OUT].getVoltage(1)==0,"Orange level mutes both headphone channels");
  top.params[CE2600::SYNC].setValue(1);
  bool sawSync=false;
  for(int i=0;i<1000;++i){
   float previous=top.osc[0].phase;
   step();
   if(top.osc[0].phase<previous && top.osc[1].phase<.01) sawSync=true;
  }
  check(sawSync,"Orange hard sync resets VCO 2 at VCO 1 wrap");
  bottom.params[CE2600::GLIDE_ON].setValue(1);
  connect(bottom,CE2600::PITCH,2);
  step();
  check(top.glidePitch>-1 && top.glidePitch<2,"Orange portamento glides between notes");
  std::puts("PASS Orange linking, keyboard, LFO, mixer taps, output copies, switch lights, unipolar FM/PWM, headphones, hard sync and glide");
 }
 CE2600 demo;defaults(demo);demo.params[CE2600::OSC+12].setValue(.24f);demo.params[CE2600::OSC+7].setValue(.007f);demo.params[CE2600::RES].setValue(.5f);
 engine::Module::ProcessArgs args;args.sampleRate=48000;args.sampleTime=1/48000.f;
 engine::Module::SampleRateChangeEvent change;change.sampleRate=48000;change.sampleTime=args.sampleTime;demo.onSampleRateChange(change);
 std::vector<float> samples;float notes[]={-1,-.416667f,-.25f,0,-.25f,-.416667f,-.833333f,-.416667f};
 for(int t=0;t<48000*8;++t){int step=t/24000;connect(demo,CE2600::PITCH,notes[step%8]);connect(demo,CE2600::GATE,t%24000<15000?10:0);demo.process(args);samples.push_back(demo.outputs[CE2600::LEFT_OUT].getVoltage());}
 wav("build/ce-2600-demo.wav",samples,48000);
 std::puts("Rendered build/ce-2600-demo.wav");
}
