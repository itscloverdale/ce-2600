#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include "ownershipOrange.hpp"

// A bus is private to one Rack Context (one patch/Pro instance). Only atomic
// samples cross audio threads. No allocation, locks, or UI access in process().
namespace panel2600 {
struct Slot {
 std::atomic<uint64_t> owner{0};
 std::atomic<bool> ready{false};
 std::atomic<float> params[CE2600::PARAMS_LEN];
 std::atomic<float> inputs[CE2600::INPUTS_LEN];
 std::atomic<bool> connected[CE2600::INPUTS_LEN];
 Slot(){for(auto& p:params)p.store(0);for(auto& i:inputs)i.store(0);for(auto& c:connected)c.store(false);}
};
struct Bus {
 Slot slots[3];std::atomic<float> phonesRight{0};std::atomic<bool> switchState{false};std::atomic<float> outputs[CE2600::OUTPUTS_LEN];
 Bus(){for(auto& o:outputs)o.store(0);}
};
struct Buses { Bus groups[16]; };
inline std::shared_ptr<Buses> contextBuses(void* context) {
 static std::mutex mutex;static std::map<void*,std::weak_ptr<Buses>> registry;
 std::lock_guard<std::mutex> lock(mutex);auto& weak=registry[context];auto result=weak.lock();
 if(!result){result=std::make_shared<Buses>();weak=result;}return result;
}
inline uint64_t nextToken(){static std::atomic<uint64_t> value{1};return value.fetch_add(1);}
}
struct CE2600Part : CE2600 {
 const int panel; const uint64_t token=panel2600::nextToken();
 std::atomic<int> group{0};std::atomic<bool> linked{false},conflict{false};
 std::shared_ptr<panel2600::Buses> buses;
 int active=-1;
 int paramOwner(int id) const {return parameterPanelOrange(id);}
 int inputOwner(int id) const {return inputPanelOrange(id);}
 CE2600Part(int panel):panel(panel),buses(panel2600::contextBuses(contextGet())){
  hardwareLayout=true;
  orangeLayout=true;
  configSwitch(GATE_SELECT,0,2,0,"Envelope gate source",{"Keyboard gate","S/H gate","Trigger"});
  // Tooltips follow the hardware normals printed on the three-row panel.
  paramQuantities[REVERB_MIX]->name="Reverb left level";
  for(int i=0;i<3;++i)paramQuantities[OSC+i*7+6]->name="VCO "+std::to_string(i+1)+" range (Audio: keyboard on, LF: keyboard off)";
  inputInfos[PITCH]->name="KBD CV IN: keyboard pitch for all three VCOs, filter tracking and processor 4 (1 V/oct)";
  for(int i=0;i<3;++i)inputInfos[OSC_IN+i*4]->name="VCO "+std::to_string(i+1)+" KBD CV: replaces KBD CV IN for this VCO only (1 V/oct)";inputInfos[OSC_IN+5]->name="VCO 2 exponential FM (normal: VCO 1 square)";
  inputInfos[OSC_IN+3]->name="VCO 1 filter mixer (normal: square)";inputInfos[OSC_IN+7]->name="VCO 2 filter mixer (normal: pulse)";
  inputInfos[ADSR_GATE]->name="Envelope gate when the gate switch is down (normal: S/H clock)";inputInfos[LAG_IN]->name="Lag (normal: envelope follower)";
  inputInfos[MIX_DIRECT_IN]->name="Mixer direct 1 (unattenuated)";inputInfos[MIX_DIRECT_IN+1]->name="Mixer direct 2 (unattenuated)";
  outputInfos[VP_OUT]->name="Inverter: -(2 + 1 + 4 + 3)";outputInfos[VP_OUT+2]->name="Inverter: -(6 + 5)";
  outputInfos[SWITCH_B_OUT]->name="Electronic switch B (A or C, alternating on S/H clock)";outputInfos[KEY_OUT]->name="Keyboard CV copy";
  inputInfos[OSC_IN+1]->name="VCO 1 exponential FM (normal: delayed LFO sine)";
  inputInfos[UPPER_CV]->name="Upper voice CV in (normal: keyboard CV)";
 }
 ~CE2600Part() override {release();}
 void release(){
  if(active>=0){auto& slot=buses->groups[active].slots[panel];if(slot.owner.load()==token){slot.ready.store(false);slot.owner.store(0);}}
  active=-1;linked.store(false);
 }
 void onRemove(const RemoveEvent& e) override {release();CE2600::onRemove(e);}
 void processBypass(const ProcessArgs& args) override {release();for(auto& o:outputs)o.setVoltage(0);}
 json_t* dataToJson() override {json_t* j=json_object();json_object_set_new(j,"instrumentLink",json_integer(group.load()+1));return j;}
 void dataFromJson(json_t* j) override {json_t* v=json_object_get(j,"instrumentLink");if(json_is_integer(v))group.store(std::max(0,std::min(15,int(json_integer_value(v))-1)));}
 void process(const ProcessArgs& args) override {
  if(panel==2){bool on=params[POWER].getValue()>.5f;lights[ON_LIGHT].setBrightness(on);lights[STANDBY_LIGHT].setBrightness(!on);}
  int requested=std::max(0,std::min(15,group.load()));if(requested!=active){release();active=requested;}
  auto& bus=buses->groups[active];auto& local=bus.slots[panel];
  uint64_t expected=0;local.owner.compare_exchange_strong(expected,token);
  if(local.owner.load()!=token){conflict.store(true);linked.store(false);for(auto& o:outputs)o.setVoltage(0);return;}
  conflict.store(false);
  for(int i=0;i<PARAMS_LEN;++i)if(paramOwner(i)==panel)local.params[i].store(params[i].getValue(),std::memory_order_relaxed);
  for(int i=0;i<INPUTS_LEN;++i)if(inputOwner(i)==panel){local.inputs[i].store(inputs[i].getVoltage(),std::memory_order_relaxed);local.connected[i].store(inputs[i].isConnected(),std::memory_order_relaxed);}
  local.ready.store(true,std::memory_order_release);
  bool complete=true;for(auto& slot:bus.slots)complete &= slot.ready.load(std::memory_order_acquire);
  linked.store(complete);
  if(panel==0){
   for(int i=0;i<PARAMS_LEN;++i)if(paramOwner(i)!=0){auto& source=bus.slots[paramOwner(i)];params[i].setValue(source.ready.load(std::memory_order_acquire)?source.params[i].load(std::memory_order_relaxed):paramQuantities[i]->getDefaultValue());}
   for(int i=0;i<INPUTS_LEN;++i)if(inputOwner(i)!=0){auto& source=bus.slots[inputOwner(i)];bool valid=source.ready.load(std::memory_order_acquire);inputs[i].channels=valid&&source.connected[i].load(std::memory_order_relaxed)?1:0;inputs[i].setVoltage(inputs[i].channels?source.inputs[i].load(std::memory_order_relaxed):0);}
   CE2600::process(args);bus.switchState.store(switchState,std::memory_order_relaxed);bus.phonesRight.store(outputs[PHONES_OUT].getVoltage(1),std::memory_order_relaxed);for(int i=0;i<OUTPUTS_LEN;++i)bus.outputs[i].store(outputs[i].getVoltage(),std::memory_order_relaxed);
  }else{
   bool top=bus.slots[0].ready.load(std::memory_order_acquire);
   for(int i=0;i<OUTPUTS_LEN;++i)outputs[i].setVoltage(top?bus.outputs[i].load(std::memory_order_relaxed):0);
   outputs[PHONES_OUT].setChannels(2);outputs[PHONES_OUT].setVoltage(top?bus.phonesRight.load(std::memory_order_relaxed):0,1);
   bool state=bus.switchState.load(std::memory_order_relaxed);lights[SWITCH_A_LIGHT].setBrightness(top&&!state);lights[SWITCH_B_LIGHT].setBrightness(top&&state);
  }
 }
};
// Slider caps are the panel's LED colours: CAP 1-5 = blue, green, red, yellow, white.
template<int LEN,bool INVERT=false,int CAP=1>
struct VSlider2600 : app::SvgSlider {
 static_assert(CAP>=1&&CAP<=5,"slider cap colour 1-5");
 VSlider2600(){
  setHandleSvg(Svg::load(asset::plugin(pluginInstance,"res/OrangeCapV"+std::to_string(CAP-1)+".svg")));
  box.size=fb->box.size=Vec(20,LEN);Vec low(10,LEN-9.f),high(10,9.f);
  if(INVERT)std::swap(low,high);
  setHandlePosCentered(low,high);
 }
};
template<int LEN,int CAP=1>
struct HSlider2600 : app::SvgSlider {
 static_assert(CAP>=1&&CAP<=5,"slider cap colour 1-5");
 HSlider2600(){
  setHandleSvg(Svg::load(asset::plugin(pluginInstance,"res/OrangeCapH"+std::to_string(CAP-1)+".svg")));
  box.size=fb->box.size=Vec(LEN,20);horizontal=true;setHandlePosCentered(Vec(10,10),Vec(LEN-10.f,10));
 }
};
// Orange panel controls: dark-nut jacks, black lever switches, red buttons, silver knob and rocker.
struct JackOrange2600 : app::SvgPort {JackOrange2600(){setSvg(Svg::load(asset::plugin(pluginInstance,"res/JackOrange.svg")));}};
struct BigJackOrange2600 : app::SvgPort {BigJackOrange2600(){setSvg(Svg::load(asset::plugin(pluginInstance,"res/BigJackOrange.svg")));}};
struct SwitchOrange2600 : app::SvgSwitch {SwitchOrange2600(){addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeSwitchUp.svg")));addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeSwitchDown.svg")));}};
struct SwitchOrangeInverted2600 : app::SvgSwitch {SwitchOrangeInverted2600(){addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeSwitchDown.svg")));addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeSwitchUp.svg")));}};
struct RangeOrange2600 : app::SvgSwitch {RangeOrange2600(){for(int i=0;i<3;++i)addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeRange"+std::to_string(i)+".svg")));}};
struct RangeOrangeInverted2600 : app::SvgSwitch {RangeOrangeInverted2600(){for(int i=2;i>=0;--i)addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeRange"+std::to_string(i)+".svg")));}};
struct ButtonOrange2600 : app::SvgSwitch {ButtonOrange2600(){momentary=true;addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeButtonUp.svg")));addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeButtonDown.svg")));}};
struct RockerOrange2600 : app::SvgSwitch {RockerOrange2600(){addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeRockerOff.svg")));addFrame(Svg::load(asset::plugin(pluginInstance,"res/OrangeRockerOn.svg")));}};
struct KnobOrange2600 : app::SvgKnob {KnobOrange2600(){minAngle=-.75f*M_PI;maxAngle=.75f*M_PI;setSvg(Svg::load(asset::plugin(pluginInstance,"res/OrangeKnob.svg")));}};
struct OrangeLight2600 : GrayModuleLightWidget {OrangeLight2600(){addBaseColor(nvgRGB(0xfc,0x97,0x02));}};
// Silent while linked; only a missing or duplicate panel is reported.
struct Link2600Display : Widget {
 CE2600Part* module=nullptr;
 void draw(const DrawArgs& args) override {
  if(!module||(module->linked.load()&&!module->conflict.load()))return;
  std::string label="LINK "+std::to_string(module->group.load()+1)+(module->conflict.load()?": DUPLICATE PANEL":": WAITING FOR PANELS");
  nvgBeginPath(args.vg);nvgRoundedRect(args.vg,0,0,box.size.x,box.size.y,3);nvgFillColor(args.vg,nvgRGB(200,201,195));nvgFill(args.vg);
  nvgStrokeColor(args.vg,nvgRGB(155,28,20));nvgStrokeWidth(args.vg,1);nvgStroke(args.vg);
  nvgFontSize(args.vg,10);nvgFontFaceId(args.vg,APP->window->uiFont->handle);nvgFillColor(args.vg,nvgRGB(155,28,20));nvgText(args.vg,6,12,label.c_str(),nullptr);
 }
};
struct CE2600PartWidget : ModuleWidget {
 // Without Rack's panel border the three rows join without a visible seam.
 void setPanel2600(const std::string& file){auto* panel=createPanel(asset::plugin(pluginInstance,file));panel->panelBorder->hide();setPanel(panel);}
 void finish(CE2600Part* module,int panel){
  // The panel print draws its own screws, only at the outer corners, so the stacked rows read as one panel.
  auto* status=new Link2600Display;status->module=module;status->box.pos=Vec(box.size.x/2-90,panel==2?2:362);status->box.size=Vec(180,16);addChild(status);
 }
 void appendContextMenu(Menu* menu) override {
  auto* m=dynamic_cast<CE2600Part*>(module);if(!m)return;
  menu->addChild(new MenuSeparator);menu->addChild(createMenuLabel("Use the same link on Top, Middle and Bottom"));
  menu->addChild(createIndexSubmenuItem("Instrument link",{"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"},[m](){return m->group.load();},[m](int value){m->group.store(value);}));
 }
};
struct CE2600OrangeTop : CE2600Part {
 CE2600OrangeTop():CE2600Part(0){}
};
struct CE2600OrangeTopWidget : CE2600PartWidget {
 CE2600OrangeTopWidget(CE2600OrangeTop* module){
  setModule(module);
  setPanel2600("res/CE2600OrangeTop.svg");
  #include "layoutOrangeTop.inc"
  finish(module,0);
 }
};
Model* modelCE2600OrangeTop=createModel<CE2600OrangeTop,CE2600OrangeTopWidget>("CE2600OrangeTop");

struct CE2600OrangeMiddle : CE2600Part {
 CE2600OrangeMiddle():CE2600Part(1){}
};
struct CE2600OrangeMiddleWidget : CE2600PartWidget {
 CE2600OrangeMiddleWidget(CE2600OrangeMiddle* module){
  setModule(module);
  setPanel2600("res/CE2600OrangeMiddle.svg");
  #include "layoutOrangeMiddle.inc"
  finish(module,1);
 }
};
Model* modelCE2600OrangeMiddle=createModel<CE2600OrangeMiddle,CE2600OrangeMiddleWidget>("CE2600OrangeMiddle");

struct CE2600OrangeBottom : CE2600Part {
 CE2600OrangeBottom():CE2600Part(2){}
};
struct CE2600OrangeBottomWidget : CE2600PartWidget {
 CE2600OrangeBottomWidget(CE2600OrangeBottom* module){
  setModule(module);
  setPanel2600("res/CE2600OrangeBottom.svg");
  #include "layoutOrangeBottom.inc"
  finish(module,2);
 }
};
Model* modelCE2600OrangeBottom=createModel<CE2600OrangeBottom,CE2600OrangeBottomWidget>("CE2600OrangeBottom");

void init(Plugin* p){
 pluginInstance=p;
 for(auto* model:{modelCE2600OrangeTop,modelCE2600OrangeMiddle,modelCE2600OrangeBottom})p->addModel(model);
}
