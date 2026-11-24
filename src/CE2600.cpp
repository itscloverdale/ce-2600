#include <rack.hpp>
#include "dsp.hpp"
using namespace rack;
Plugin* pluginInstance;
struct CE2600 : Module {
 enum Params { ENUMS(OSC,21), CUTOFF, RES, FILTER_EG, KEYTRACK, RING_LEVEL, NOISE_LEVEL, EXT_LEVEL,
  VCA_BIAS, VCA_ENV, VCA_AR, VOLUME, PRE_GAIN, RING_X, RING_Y, RING_MODE, NOISE_COLOR, LAG_TIME,
  CLOCK_RATE, SH_LEVEL, A,D,S,R, AR_A, AR_R, MANUAL, REVERB_MIX, REVERB_SEND, ENUMS(FM_SH,3), ENUMS(FM_EG,3), ENUMS(KEY_ENABLE,3), FILTER_FINE, FILTER_FM, VCA_FILTER, VCA_RING, MIX_FILTER, MIX_VCA, PAN, SPEAKER_L, SPEAKER_R, ENUMS(VP_GAIN,3), NOISE_LEVEL_OUT, FOLLOW_GAIN, PRE_RANGE, GATE_SELECT, POWER, REVERB_LEVEL_R, ENUMS(SYNC,2), FILTER_MODE, TIME_FACTOR, GLIDE, GLIDE_ON, GLIDE_HOLD, LFO_RATE, VIB_DELAY, VIB_DEPTH, DUO, REPEAT, MULTI_TRIG, PHONES_LEVEL, AR_TIME_FACTOR, PARAMS_LEN };
 enum Inputs { PITCH, GATE, TRIG, ENUMS(OSC_IN,12), FILTER_IN, FILTER_CV, FILTER_EG_IN, VCA_IN, VCA_CV,
  PRE_IN, RING_X_IN, RING_Y_IN, LAG_IN, SH_IN, CLOCK_IN, ADSR_GATE, AR_GATE, REVERB_IN, INV_IN, ENUMS(FM_SH_IN,3), ENUMS(FM_EG_IN,3), FILTER_FM_IN, VCA_RING_IN, MIX_FILTER_IN, MIX_VCA_IN, ENUMS(VP_IN,6), MULT_IN, SWITCH_A_IN, SWITCH_B_IN, SWITCH_C_IN, RING_MIX_IN, NOISE_MIX_IN, AR_VCA_IN, FOLLOW_IN, LEFT_IN, RIGHT_IN, PAN_IN, ENUMS(MIX_DIRECT_IN,2), UPPER_CV, VIB_IN, ADSR_GATE_IN, INPUTS_LEN };
 enum Outputs { ENUMS(OSC_OUT,12), FILTER_OUT, VCA_OUT, LEFT_OUT, RIGHT_OUT, PRE_OUT, FOLLOW_OUT, RING_OUT, NOISE_OUT,
  LAG_OUT, SH_OUT, CLOCK_OUT, ADSR_OUT, AR_OUT, INV_OUT, REVERB_L, REVERB_R, ENUMS(VP_OUT,3), ENUMS(MULT_OUT,4), KEY_OUT, SWITCH_A_OUT, SWITCH_B_OUT, SWITCH_C_OUT, INPUT_PITCH_OUT, PHONES_OUT, ENUMS(LFO_OUT,3), UPPER_OUT, KEY_GATE_OUT, KEY_TRIG_OUT, MIX_OUT, ENUMS(ATTEN_OUT,2), ENUMS(OUT_COPY,2), OUTPUTS_LEN };
 enum Lights { GATE_LIGHT, CLOCK_LIGHT, ENV_LIGHT, LEVEL_LIGHT, ON_LIGHT, STANDBY_LIGHT, SWITCH_A_LIGHT, SWITCH_B_LIGHT, LIGHTS_LEN };
 bool hardwareLayout=false, orangeLayout=false;
 float lfoPhase=0, glidePitch=0, vibratoAge=0, keyPulse=0, lastKeyboard=0;
 bool glideInitialized=false;
 ce2600::Oscillator osc[3]; ce2600::Envelope adsr, ar; ce2600::Ladder filter; ce2600::Noise noise; ce2600::Reverb reverb;
 ce2600::Trigger gateTrigger, envGateTrigger, arGateTrigger, retrigger, shTrigger;
 float lag=0, follow=0, held=0, clockPhase=0, clockPulse=0, prevSine=0; bool switchState=false;
 CE2600() {
  config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
  for(int i=0;i<3;++i) {
   int p=OSC+i*7;
   std::string n="VCO "+std::to_string(i+1)+" ";
   configParam(p,-5,5,i==1 ? -1.f : 0.f,n+"coarse pitch"," oct");
   configParam(p+1,-1,1,0,n+"fine pitch"," semitones");
   configParam(p+2,.03,.97,.5,n+"pulse width","%",0,100);
   // FM and PWM sliders are attenuators: no modulation at the bottom, full depth at the top.
   configParam(p+3,0,1,0,n+"FM depth","%",0,100); configParam(p+4,0,1,0,n+"PWM depth","%",0,100);
   configParam(p+5,0,1,i==0?.65f:0.f,n+"filter mixer level");
   configSwitch(p+6,0,1,0,n+"frequency range",{"Audio","Low"});
   const char* inNames[]={"1 V/oct (normal: keyboard)","exponential FM (normal: VCO 2 sine)","PWM (normal: ADSR)","filter mixer (normal: saw)"};
   const char* outNames[]={"saw","pulse","sine","triangle"};
   for(int j=0;j<4;++j){configInput(OSC_IN+i*4+j,n+inNames[j]);configOutput(OSC_OUT+i*4+j,n+outNames[j]);}
  }
  configParam(CUTOFF,std::log2(20.f),std::log2(18000.f),std::log2(2400.f),"Filter cutoff"," Hz",2);
  configParam(RES,0,1,.15,"Filter resonance"); configParam(FILTER_EG,-5,5,1.5,"Filter envelope depth"," oct");
  configParam(KEYTRACK,0,1,1,"Filter keyboard tracking");configParam(RING_LEVEL,0,1,0,"Ring mixer level");
  configParam(NOISE_LEVEL,0,1,0,"Noise mixer level");configParam(EXT_LEVEL,0,1,0,"External mixer level");
  configParam(VCA_BIAS,0,1,0,"VCA initial gain (drone)");configParam(VCA_ENV,0,1,1,"VCA ADSR amount");
  configParam(VCA_AR,0,1,0,"VCA AR amount");configParam(VOLUME,0,1,.7,"Master level");
  configParam(PRE_GAIN,0,2,0,"Preamp gain","x",10);
  configParam(RING_X,0,1,1,"Ring X level");configParam(RING_Y,0,1,1,"Ring Y level");
  configSwitch(RING_MODE,0,1,0,"Ring coupling",{"AC","DC"});
  configParam(NOISE_COLOR,0,1,.3,"Noise color (white to dark)");
  configParam(LAG_TIME,0,1,.35,"Lag time"," s",10000,.001);
  configParam(CLOCK_RATE,-3,6,1,"Sample clock rate"," Hz",2);
  configParam(SH_LEVEL,0,1,1,"Sample and hold level");
  configParam(A,0,1,.25,"ADSR attack"," s",10000,.001);configParam(D,0,1,.5,"ADSR decay"," s",10000,.001);
  configParam(S,0,1,.65,"ADSR sustain","%",0,100);configParam(R,0,1,.45,"ADSR release"," s",10000,.001);
  configParam(AR_A,0,1,.15,"AR attack"," s",10000,.001);configParam(AR_R,0,1,.5,"AR release"," s",10000,.001);
  configButton(MANUAL,"Manual gate");configParam(REVERB_MIX,0,1,.18,"Reverb wet mix");configParam(REVERB_SEND,0,1,.65,"Reverb send");
  for(int i=0;i<3;++i){
   configParam(FM_SH+i,0,1,0,"VCO "+std::to_string(i+1)+" S&H FM amount");
   configParam(FM_EG+i,0,1,0,"VCO "+std::to_string(i+1)+" ADSR FM amount");
   configSwitch(KEY_ENABLE+i,0,1,1,"VCO "+std::to_string(i+1)+" keyboard tracking",{"Off","On"});
   configInput(FM_SH_IN+i,"VCO "+std::to_string(i+1)+" FM (normal: sample and hold)");
   configInput(FM_EG_IN+i,"VCO "+std::to_string(i+1)+" FM (normal: ADSR)");
   configParam(VP_GAIN+i,0,1,1,"Voltage processor "+std::to_string(i+1)+" level");
   configOutput(VP_OUT+i,"Voltage processor "+std::to_string(i+1)+" inverted sum");
  }
  configParam(FILTER_FINE,-1,1,0,"Filter fine tune"," semitones");configParam(FILTER_FM,0,1,0,"Filter VCO 2 FM amount");
  configInput(FILTER_FM_IN,"Filter FM (normal: VCO 2 sine)");
  configParam(VCA_FILTER,0,1,1,"VCA filter level");configParam(VCA_RING,0,1,0,"VCA ring level");configInput(VCA_RING_IN,"VCA ring (normal: ring modulator)");
  configParam(MIX_FILTER,0,1,0,"Output mixer filter level");configParam(MIX_VCA,0,1,1,"Output mixer VCA level");
  configInput(MIX_FILTER_IN,"Output mixer filter (normal: VCF)");configInput(MIX_VCA_IN,"Output mixer VCA (normal: VCA)");
  configParam(PAN,0,1,.5,"Stereo pan");configParam(SPEAKER_L,0,1,1,"Left speaker level");configParam(SPEAKER_R,0,1,1,"Right speaker level");
  for(int i=0;i<6;++i)configInput(VP_IN+i,"Voltage processor input "+std::to_string(i+1));
  configInput(MULT_IN,"Multiple source");for(int i=0;i<4;++i)configOutput(MULT_OUT+i,"Multiple copy "+std::to_string(i+1));
  configOutput(KEY_OUT,"Keyboard pitch copy");configOutput(INPUT_PITCH_OUT,"Keyboard pitch second copy");
  configInput(SWITCH_A_IN,"Electronic switch A");configInput(SWITCH_B_IN,"Electronic switch B");configInput(SWITCH_C_IN,"Electronic switch C");
  configOutput(SWITCH_A_OUT,"Electronic switch A");configOutput(SWITCH_B_OUT,"Electronic switch B");configOutput(SWITCH_C_OUT,"Electronic switch C");
  configParam(FOLLOW_GAIN,0,1,1,"Envelope follower input level");configInput(FOLLOW_IN,"Envelope follower (normal: preamp)");
  configParam(NOISE_LEVEL_OUT,0,1,1,"Noise generator output level");configInput(RING_MIX_IN,"Filter ring mixer (normal: ring modulator)");configInput(NOISE_MIX_IN,"Filter noise mixer (normal: noise)");configInput(AR_VCA_IN,"VCA linear control (normal: AR)");
  configSwitch(PRE_RANGE,0,2,1,"Preamp range",{"x10","x100","x1000"});configSwitch(GATE_SELECT,0,1,0,"Envelope gate source",{"Keyboard","S/H GATE jack"});
  configSwitch(POWER,0,1,1,"Power",{"Standby","On"});configParam(REVERB_LEVEL_R,0,1,.18,"Reverb right level");
  configInput(LEFT_IN,"Left output direct");configInput(RIGHT_IN,"Right output direct");configInput(PAN_IN,"Pan (normal: mixer)");
  configInput(MIX_DIRECT_IN,"Mixer direct 1");configInput(MIX_DIRECT_IN+1,"Mixer direct 2");
  configOutput(PHONES_OUT,"Stereo phones (2 channels: left, right)");
  configInput(PITCH,"Keyboard pitch (1 V/oct)");configInput(GATE,"Keyboard gate");configInput(TRIG,"ADSR retrigger");
  configInput(FILTER_IN,"External filter mixer audio");configInput(FILTER_CV,"Filter CV (1 V/oct)");
  configInput(FILTER_EG_IN,"Filter envelope (normal: ADSR)");configInput(VCA_IN,"VCA audio (normal: filter)");
  configInput(VCA_CV,"VCA envelope (normal: ADSR)");configInput(PRE_IN,"Preamp audio");
  configInput(RING_X_IN,"Ring X (normal: VCO 1 saw)");configInput(RING_Y_IN,"Ring Y (normal: VCO 2 sine)");
  configInput(LAG_IN,"Lag CV (normal: keyboard pitch)");configInput(SH_IN,"Sample source (normal: noise)");
  configInput(CLOCK_IN,"Sample clock (normal: internal clock)");configInput(ADSR_GATE,"ADSR gate (normal: keyboard)");
  configInput(AR_GATE,"AR gate (normal: keyboard)");configInput(REVERB_IN,"Reverb audio (normal: VCA)");
  configInput(INV_IN,"Inverter CV (normal: ADSR)");
  const char* names[]={"Filter","VCA (dry)","Main left / mono","Main right","Preamp","Envelope follower","Ring modulator","Noise","Lag","Sample and hold","Internal clock","ADSR","AR","Inverter","Reverb left (wet)","Reverb right (wet)"};
  for(int i=0;i<16;++i)configOutput(FILTER_OUT+i,names[i]);
  configBypass(VCA_IN,LEFT_OUT);configBypass(VCA_IN,RIGHT_OUT);
  configSwitch(SYNC,0,1,0,"VCO 2 hard sync to VCO 1",{"Off","On"});
  configSwitch(SYNC+1,0,1,0,"VCO 3 hard sync to VCO 1",{"Off","On"});
  configSwitch(FILTER_MODE,0,1,0,"Filter mode",{"4012","4072"});
  configSwitch(TIME_FACTOR,0,2,1,"Envelope time factor",{"×0.5","×1","×2"});
  configSwitch(AR_TIME_FACTOR,0,2,1,"AR time factor",{"×0.5","×1","×2"});
  configInput(ADSR_GATE_IN,"ADSR gate override");
  configParam(GLIDE,0,1,.4f,"Portamento time");
  configSwitch(GLIDE_ON,0,1,0,"Portamento",{"Off","On"});
  configButton(GLIDE_HOLD,"Momentary portamento");
  configParam(LFO_RATE,-2,4.643856f,1,"LFO speed"," Hz",2);
  configParam(VIB_DELAY,0,1,0,"Vibrato delay"," s",0,4);
  configParam(VIB_DEPTH,0,1,0,"Vibrato depth");
  configSwitch(DUO,0,1,0,"Voice mode",{"Mono","Duo"});
  configSwitch(REPEAT,0,2,1,"Trigger repeat",{"Keyboard","Off","Auto"});
  configSwitch(MULTI_TRIG,0,1,0,"Trigger mode",{"Single","Multi"});
  configParam(PHONES_LEVEL,0,1,.7f,"Headphone level");
  configInput(UPPER_CV,"Upper voice pitch (defaults to channel 2 of keyboard CV)");
  configInput(VIB_IN,"External vibrato");
  configOutput(LFO_OUT,"LFO triangle");configOutput(OUT_COPY,"Left output (3.5 mm copy)");configOutput(OUT_COPY+1,"Right output (3.5 mm copy)");configOutput(LFO_OUT+1,"LFO square");configOutput(LFO_OUT+2,"LFO delayed sine");
  configOutput(UPPER_OUT,"Upper voice CV");configOutput(KEY_GATE_OUT,"Keyboard gate");configOutput(KEY_TRIG_OUT,"Keyboard trigger");
  configOutput(MIX_OUT,"Mixer output");configOutput(ATTEN_OUT,"Filter post-attenuator");configOutput(ATTEN_OUT+1,"VCA post-attenuator");
  reverb.setup(44100);
 }
 void onSampleRateChange(const SampleRateChangeEvent& e) override { reverb.setup(e.sampleRate); }
 void onReset(const ResetEvent& e) override {
  Module::onReset(e);for(auto& o:osc)o={};adsr={};ar={};filter={};noise={};
  gateTrigger={};envGateTrigger={};arGateTrigger={};retrigger={};shTrigger={};
  glideInitialized=false;lfoPhase=glidePitch=vibratoAge=keyPulse=lastKeyboard=0;
  switchState=false;lag=follow=held=clockPhase=clockPulse=prevSine=ringDcX=ringDcY=0;reverb.setup(APP->engine->getSampleRate());
 }
 float ringDcX=0,ringDcY=0;
 void process(const ProcessArgs& args) override {
  auto p=[&](int id){return params[id].getValue();};
  auto in=[&](int id,float normal=0.f){return ce2600::safe(inputs[id].getNormalVoltage(normal));};
  auto out=[&](int id,float v){outputs[id].setVoltage(ce2600::safe(v));};
  float dt=args.sampleTime;
  bool keyEdge=gateTrigger.process(std::max(in(GATE),p(MANUAL)*10));
  float gate=gateTrigger.high?10:0;
  float keyboardPitch=in(PITCH), upperPitch=keyboardPitch, lfoSine=0, keyTrig=0;
  if(orangeLayout){
   if(!glideInitialized || (p(GLIDE_ON)<.5f && p(GLIDE_HOLD)<.5f)) glidePitch=keyboardPitch;
   else glidePitch+=(1-std::exp(-dt/ce2600::time(p(GLIDE))))*(keyboardPitch-glidePitch);
   glideInitialized=true;
   upperPitch=p(DUO)>.5f?std::max(keyboardPitch,in(UPPER_CV,keyboardPitch)):glidePitch;
   lfoPhase+=dt*std::exp2(p(LFO_RATE));
   bool repeatEdge=lfoPhase>=1.f;
   if(repeatEdge)lfoPhase-=1.f;
   if(keyEdge||!gate)vibratoAge=0;
   else vibratoAge=std::min(4.f,vibratoAge+dt);
   float fade=p(VIB_DELAY)>0?std::min(1.f,vibratoAge/(4*p(VIB_DELAY))):1.f;
   lfoSine=std::sin(2*ce2600::pi*lfoPhase)*fade*5.f;
   if(keyEdge || (gate && p(MULTI_TRIG)>.5f && keyboardPitch!=lastKeyboard)
      || (repeatEdge && (p(REPEAT)>1.5f || (p(REPEAT)<.5f && gate)))) keyPulse=.001f;
   lastKeyboard=keyboardPitch;
   keyTrig=keyPulse>0?10.f:0.f;
   keyPulse=std::max(0.f,keyPulse-dt);
   out(LFO_OUT,5-20*std::fabs(lfoPhase-.5f));out(LFO_OUT+1,lfoPhase<.5f?5:-5);out(LFO_OUT+2,lfoSine);
   out(UPPER_OUT,upperPitch);out(KEY_GATE_OUT,gate);out(KEY_TRIG_OUT,keyTrig);
   keyboardPitch=glidePitch+(lfoSine+in(VIB_IN))*.02f*p(VIB_DEPTH);
  }
  float envGate=in(ADSR_GATE,gate),arGate=in(AR_GATE,gate);
  if(hardwareLayout)envGate=arGate=p(GATE_SELECT)<.5f?gate:in(ADSR_GATE,clockPhase<.5f?10.f:0.f);
  if(orangeLayout){
   int route=int(p(GATE_SELECT)+.5f);
   arGate=route==0?gate:(route==1?in(ADSR_GATE,clockPhase<.5f?10.f:0.f):in(TRIG,keyTrig));
   envGate=in(ADSR_GATE_IN,arGate);
  }
  const float timeFactor=orangeLayout?std::exp2(p(TIME_FACTOR)-1):1;
  const float arTimeFactor=orangeLayout?std::exp2(p(AR_TIME_FACTOR)-1):1;
  envGateTrigger.process(std::max(envGate,p(MANUAL)*10));
  arGateTrigger.process(std::max(arGate,p(MANUAL)*10));
  float eg=adsr.process(envGateTrigger.high,retrigger.process(in(TRIG,keyTrig)),ce2600::time(p(A))*timeFactor,ce2600::time(p(D))*timeFactor,p(S),ce2600::time(p(R))*timeFactor,dt);
  float arEg=ar.process(arGateTrigger.high,false,ce2600::time(p(AR_A))*arTimeFactor,.001f,1,ce2600::time(p(AR_R))*arTimeFactor,dt);
  ce2600::Oscillator::Waves waves[3];
  bool masterWrapped=false;
  for(int i=0;i<3;++i){
   int b=OSC+i*7, j=OSC_IN+i*4;
   // On the hardware panel the AUDIO/LF switch also connects or disconnects the keyboard.
   float keyboard=hardwareLayout?in(j,p(b+6)<.5f?in(PITCH):0.f):in(j,in(PITCH))*p(KEY_ENABLE+i);
   if(orangeLayout)keyboard=in(j,p(b+6)>.5f?0.f:(i==2 && p(DUO)>.5f?upperPitch:keyboardPitch));
   float fmNormal=hardwareLayout&&i==1?waves[0].pulse:prevSine;
   if(orangeLayout&&i==0)fmNormal=lfoSine;
   float pitch=p(b)+p(b+1)/12+keyboard+p(b+3)*in(j+1,fmNormal)*.4f+in(FM_SH_IN+i,hardwareLayout&&i==2?outputs[NOISE_OUT].getVoltage():held)*p(FM_SH+i)*.4f+in(FM_EG_IN+i,eg)*p(FM_EG+i)*.4f;
   float hz=(p(b+6)>.5f?2.f:261.625565f)*std::exp2(ce2600::limit(pitch,-16,12));
   if(orangeLayout && i>0 && masterWrapped && p(SYNC+i-1)>.5f)osc[i]={};
   float oldPhase=osc[i].phase;
   waves[i]=osc[i].process(hz,p(b+2)+p(b+4)*in(j+2,hardwareLayout && !(orangeLayout&&i==2)?outputs[NOISE_OUT].getVoltage():eg)*.08f,dt);
   if(i==0)masterWrapped=osc[i].phase<oldPhase;
   int o=OSC_OUT+i*4;out(o,waves[i].saw);out(o+1,waves[i].pulse);out(o+2,waves[i].sine);out(o+3,waves[i].tri);
  }
  prevSine=waves[1].sine;
  const float ranges[]={.1f,1.f,10.f};
  float pre=std::tanh(in(PRE_IN)*std::pow(10,p(PRE_GAIN))*ranges[std::max(0,std::min(2,int(p(PRE_RANGE)+.5f)))]*.2f)*5;
  float followerInput=in(FOLLOW_IN,pre)*p(FOLLOW_GAIN);
  follow+=(1-std::exp(-dt/(std::fabs(followerInput)>follow?.003f:.08f)))*(std::fabs(followerInput)-follow);
  float nx=in(RING_X_IN,waves[0].saw)*p(RING_X),ny=in(RING_Y_IN,waves[1].sine)*p(RING_Y);
  float ac=1-std::exp(-2*ce2600::pi*5*dt);ringDcX+=ac*(nx-ringDcX);ringDcY+=ac*(ny-ringDcY);
  float ring=(nx-(p(RING_MODE)<.5f?ringDcX:0))*(ny-(p(RING_MODE)<.5f?ringDcY:0))*.2f;
  float n=noise.process(p(NOISE_COLOR),dt)*p(NOISE_LEVEL_OUT);
  lag+=(1-std::exp(-dt/ce2600::time(p(LAG_TIME))))*(in(LAG_IN,hardwareLayout?follow*2:in(PITCH))-lag);
  clockPhase+=dt*std::exp2(p(CLOCK_RATE));if(clockPhase>=1){clockPhase-=1;clockPulse=.001f;}
  float clk=clockPulse>0?10:0;clockPulse=std::max(0.f,clockPulse-dt);
  if(shTrigger.process(in(CLOCK_IN,clk))){held=in(SH_IN,n)*p(SH_LEVEL);switchState=!switchState;}
  float mix=in(RING_MIX_IN,ring)*p(RING_LEVEL)+in(NOISE_MIX_IN,n)*p(NOISE_LEVEL)+in(FILTER_IN,pre)*p(EXT_LEVEL);
  for(int i=0;i<3;++i)mix+=in(OSC_IN+i*4+3,hardwareLayout&&i<2?waves[i].pulse:waves[i].saw)*p(OSC+i*7+5);
  float cutoff=std::exp2(ce2600::limit(p(CUTOFF)+p(FILTER_FINE)/12+in(FILTER_FM_IN,prevSine)*p(FILTER_FM)*.4f+(hardwareLayout?p(KEYTRACK)*in(FILTER_CV,keyboardPitch):in(FILTER_CV)+p(KEYTRACK)*in(PITCH))+p(FILTER_EG)*in(FILTER_EG_IN,eg)*.1f,0,16));
  if(orangeLayout && p(FILTER_MODE)>.5f) cutoff=std::min(cutoff*.84f,11000.f);
  float filtered=filter.process(mix,cutoff,p(RES),args.sampleRate);
  float envVca=ce2600::limit(in(VCA_CV,eg)*.1f,0,1);if(hardwareLayout)envVca*=envVca;
  float gain=ce2600::limit(p(VCA_BIAS)+envVca*p(VCA_ENV)+in(AR_VCA_IN,arEg)*.1f*p(VCA_AR),0,1);
  float dry=(in(VCA_IN,filtered)*p(VCA_FILTER)+in(VCA_RING_IN,ring)*p(VCA_RING))*gain;
  float filterTap=in(MIX_FILTER_IN,filtered)*p(MIX_FILTER), vcaTap=in(MIX_VCA_IN,dry)*p(MIX_VCA);
  float mixed=filterTap+vcaTap;
  out(ATTEN_OUT,filterTap);out(ATTEN_OUT+1,vcaTap);out(MIX_OUT,mixed);
  float level=p(VOLUME)*(p(POWER)>.5f?1.f:0.f);std::array<float,2> wet;float l,r;
  if(hardwareLayout){
   // Hardware output section: direct mixer inputs, patchable pan input, separate left/right reverb levels and direct L/R inputs.
   mixed+=in(MIX_DIRECT_IN)+in(MIX_DIRECT_IN+1);wet=reverb.process(in(REVERB_IN,mixed));float panned=in(PAN_IN,mixed);
   l=(panned*std::sqrt(2*(1-p(PAN)))+wet[0]*p(REVERB_MIX)+in(LEFT_IN))*level*p(SPEAKER_L);
   r=(panned*std::sqrt(2*p(PAN))+wet[1]*p(REVERB_LEVEL_R)+in(RIGHT_IN))*level*p(SPEAKER_R);
  }else{
   wet=reverb.process(in(REVERB_IN,mixed)*p(REVERB_SEND));
   l=(mixed*(1-p(REVERB_MIX))+wet[0]*p(REVERB_MIX))*level*std::sqrt(2*(1-p(PAN)))*p(SPEAKER_L);
   r=(mixed*(1-p(REVERB_MIX))+wet[1]*p(REVERB_MIX))*level*std::sqrt(2*p(PAN))*p(SPEAKER_R);
  }
  outputs[PHONES_OUT].setChannels(2);outputs[PHONES_OUT].setVoltage(ce2600::safe(l*(orangeLayout?p(PHONES_LEVEL):1)),0);outputs[PHONES_OUT].setVoltage(ce2600::safe(r*(orangeLayout?p(PHONES_LEVEL):1)),1);
  out(FILTER_OUT,filtered);out(VCA_OUT,dry);out(LEFT_OUT,l);out(RIGHT_OUT,r);out(OUT_COPY,l);out(OUT_COPY+1,r);out(PRE_OUT,pre);out(FOLLOW_OUT,follow*2);
  out(RING_OUT,ring);out(NOISE_OUT,n);out(LAG_OUT,lag);out(SH_OUT,held);out(CLOCK_OUT,clk);out(ADSR_OUT,eg);out(AR_OUT,arEg);
  out(INV_OUT,-in(INV_IN,eg));out(REVERB_L,wet[0]);out(REVERB_R,wet[1]);
  for(int i=0;i<3;++i){float normal=i==0?-10.f:(i==1?in(PITCH):10.f);out(VP_OUT+i,-(in(VP_IN+i*2,normal)*p(VP_GAIN+i)+in(VP_IN+i*2+1)));}
  // Hardware: processors 2 and 4 share the first inverter with inputs 1 and 3.
  if(hardwareLayout)out(VP_OUT,-(in(VP_IN,-10)*p(VP_GAIN)+in(VP_IN+1)+in(VP_IN+2,in(PITCH))*p(VP_GAIN+1)+in(VP_IN+3)));
  for(int i=0;i<4;++i)out(MULT_OUT+i,in(MULT_IN));out(KEY_OUT,orangeLayout?keyboardPitch:in(PITCH));out(INPUT_PITCH_OUT,in(PITCH));
  out(SWITCH_A_OUT,switchState?0:in(SWITCH_B_IN));out(SWITCH_C_OUT,switchState?in(SWITCH_B_IN):0);out(SWITCH_B_OUT,switchState?in(SWITCH_C_IN):in(SWITCH_A_IN));
  lights[GATE_LIGHT].setSmoothBrightness(gate*.1f,dt);lights[CLOCK_LIGHT].setSmoothBrightness(clk*.1f,dt);
  lights[ENV_LIGHT].setSmoothBrightness(eg*.1f,dt);lights[SWITCH_A_LIGHT].setBrightness(!switchState);lights[SWITCH_B_LIGHT].setBrightness(switchState);lights[LEVEL_LIGHT].setSmoothBrightness(std::fabs(l)*.2f,dt);
 }
};
#include "ThreePanel.hpp"
