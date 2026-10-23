#pragma once
// BananaOS (github.com/JorreFog/BananaLinux) on the RG DS / RG DS Plus.
//
// BananaOS has no compositor: the launcher (banana/Yokoi.sh) starts Yokoi on SDL's dummy video driver with
// BananaOS's SDL patch (SDL_BANANA_PANELS=side), which makes one window spanning both panels and flips each half
// straight onto its panel. Yokoi draws its two screens into that window's halves (Layout below).
//
// Input follows BananaOS's emulator contract (docs/EMULATORS.md): the buttons are one evdev device, read and never
// grabbed, with fixed key codes (A = BTN_EAST, B = BTN_SOUTH, X = BTN_NORTH, Y = BTN_WEST, F = BTN_MODE ...), so no
// calibration is needed. The volume keys belong to bananad, which sets the hardware volume: Yokoi leaves them alone.
// The bottom panel's touchscreen is the event device whose sysfs path contains BANANA_TOUCH (fe5e0000.i2c).
#include <SDL.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "yokoi_input_mapping.h"

namespace banana {

// YOKOI_PLATFORM=banana is exported by the BananaOS launcher.
inline bool active(){const char*p=getenv("YOKOI_PLATFORM");return p&&!strcmp(p,"banana");}

// Where each of Yokoi's two screens goes in a window shared by both panels. Panel 0 is the top panel.
struct Layout {
 bool shared=false,side=true;
 int windowW=0,windowH=0;
 SDL_Rect panel[2]{};
};
// SDL_BANANA_PANELS (side|stack) is BananaOS's SDL patch: the dummy driver's one display is both panels together.
// YOKOI_SHARED_WINDOW does the same layout on any SDL, for headless tests; YOKOI_SHARED_SIZE=WxH sets that window.
inline Layout layout(const char*videoDriver,int displayW,int displayH){
 Layout l;const char*how=getenv("YOKOI_SHARED_WINDOW");
 if(!how||!*how){
  const char*panels=getenv("SDL_BANANA_PANELS");
  bool dummy=videoDriver&&(!strcmp(videoDriver,"dummy")||!strcmp(videoDriver,"evdev")||!strcmp(videoDriver,"offscreen"));
  if(panels&&*panels&&dummy)how=panels;
 }
 if(!how||!*how)return l;
 int w=displayW,h=displayH;
 if(const char*size=getenv("YOKOI_SHARED_SIZE")){int a=0,b=0;if(sscanf(size,"%dx%d",&a,&b)==2&&a>0&&b>0){w=a;h=b;}}
 if(w<2||h<2)return l;
 l.shared=true;l.side=strcmp(how,"stack")!=0;l.windowW=w;l.windowH=h;
 int pw=l.side?w/2:w,ph=l.side?h:h/2;
 l.panel[0]={0,0,pw,ph};
 l.panel[1]=l.side?SDL_Rect{pw,0,pw,ph}:SDL_Rect{0,ph,pw,ph};
 return l;
}
// A point normalized over the whole window, as one normalized over a panel (for SDL touch/mouse fallbacks).
inline int toPanel(const Layout&l,float&x,float&y){
 if(!l.shared)return -1;
 float px=x*l.windowW,py=y*l.windowH;int index=0;
 for(int i=0;i<2;i++){const auto&r=l.panel[i];if(px>=r.x&&px<r.x+r.w&&py>=r.y&&py<r.y+r.h)index=i;}
 const auto&r=l.panel[index];x=(px-r.x)/float(r.w);y=(py-r.y)/float(r.h);return index;
}

template<size_t N> inline bool testBit(const unsigned long(&bits)[N],unsigned n){
 const unsigned per=8*sizeof(unsigned long);return n/per<N&&(bits[n/per]>>(n%per)&1ul);
}
// The event device's sysfs path (/sys/devices/.../fe5e0000.i2c/.../input/inputN/eventM).
inline std::string sysfsPath(const std::string&node){
 std::string link="/sys/class/input/"+node.substr(node.find_last_of('/')+1);char real[PATH_MAX];
 return realpath(link.c_str(),real)?std::string(real):std::string();
}
// The bottom panel's touchscreen: BANANA_TOUCH names part of its sysfs path (YOKOI_TOUCH overrides it).
inline std::string touchMatch(){
 const char*t=getenv("YOKOI_TOUCH");if(!t||!*t)t=getenv("BANANA_TOUCH");return t&&*t?t:"fe5e0000.i2c";
}

// The handheld's buttons by their kernel codes.
class Pad {
 struct Device {int fd;std::string path;};
 std::vector<Device> devices;
 uint32_t held=0,pressed=0;
 int hatX=0,hatY=0;
 bool toggle=false,previousLetter=false,nextLetter=false;int swap=0;
 bool fHeld=false,fChord=false; // F + volume is bananad's brightness: that F is not a Pause
 static uint32_t bitFor(unsigned code){
  switch(code){
  case BTN_DPAD_UP:return CTL_DPAD_UP;case BTN_DPAD_DOWN:return CTL_DPAD_DOWN;
  case BTN_DPAD_LEFT:return CTL_DPAD_LEFT;case BTN_DPAD_RIGHT:return CTL_DPAD_RIGHT;
  case BTN_EAST:return CTL_A;case BTN_SOUTH:return CTL_B;case BTN_NORTH:return CTL_X;case BTN_WEST:return CTL_Y;
  case BTN_START:return CTL_START;case BTN_SELECT:return CTL_SELECT;case BTN_TL:return CTL_L1;
  default:return 0;}
 }
 uint32_t hatBits()const{return (hatY<0?CTL_DPAD_UP:0)|(hatY>0?CTL_DPAD_DOWN:0)|(hatX<0?CTL_DPAD_LEFT:0)|(hatX>0?CTL_DPAD_RIGHT:0);}
 void resync(Device&d){
  unsigned long keys[(KEY_MAX+8*sizeof(unsigned long))/(8*sizeof(unsigned long))]{};
  if(ioctl(d.fd,EVIOCGKEY(sizeof keys),keys)<0)return;
  for(unsigned code:{BTN_DPAD_UP,BTN_DPAD_DOWN,BTN_DPAD_LEFT,BTN_DPAD_RIGHT,BTN_EAST,BTN_SOUTH,BTN_NORTH,BTN_WEST,BTN_START,BTN_SELECT,BTN_TL}){
   uint32_t b=bitFor(code);if(testBit(keys,code))held|=b;else held&=~b;
  }
 }
public:
 ~Pad(){for(auto&d:devices)close(d.fd);}
 bool simulated=false; // tests: events come from key()/hat() only
 bool available()const{return simulated||!devices.empty();}
 // The joypad (retrogame_joypad: START, SELECT and the face buttons), and the HOME key's adc-keys device if separate.
 bool open(){
  glob_t paths{};if(glob("/dev/input/event*",0,nullptr,&paths)!=0){globfree(&paths);return false;}
  bool pad=false;
  for(size_t i=0;i<paths.gl_pathc;i++){
   int fd=::open(paths.gl_pathv[i],O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
   char name[256]{};ioctl(fd,EVIOCGNAME(sizeof name),name);
   unsigned long keys[(KEY_MAX+8*sizeof(unsigned long))/(8*sizeof(unsigned long))]{};
   if(ioctl(fd,EVIOCGBIT(EV_KEY,sizeof keys),keys)<0){close(fd);continue;}
   bool joypad=!pad&&testBit(keys,BTN_START)&&testBit(keys,BTN_SELECT)&&testBit(keys,BTN_SOUTH)&&testBit(keys,BTN_EAST);
   bool home=!joypad&&(testBit(keys,BTN_Z)||testBit(keys,BTN_MODE))&&!testBit(keys,BTN_SOUTH);
   bool volume=!joypad&&!home&&(testBit(keys,KEY_VOLUMEUP)||testBit(keys,KEY_VOLUMEDOWN)); // only to see F + volume
   if(!joypad&&!home&&!volume){close(fd);continue;}
   devices.push_back({fd,paths.gl_pathv[i]});pad|=joypad;resync(devices.back());
   printf("BananaOS input: %s (%s)%s\n",paths.gl_pathv[i],name,joypad?" joypad":home?" home key":" volume keys (left to bananad)");
  }
  globfree(&paths);
  if(!pad){for(auto&d:devices)close(d.fd);devices.clear();}
  return pad;
 }
 // One kernel key event; kept apart from the device reads for tests.
 void key(unsigned code,int value){
  if(value==2)return; // kernel autorepeat
  uint32_t b=bitFor(code);
  if(b){if(value){if(!(held&b))pressed|=b;held|=b;}else held&=~b;}
  if(code==BTN_MODE){ // F: Pause / back when released, unless it was F + volume (brightness)
   if(value==1){fHeld=true;fChord=false;}else if(value==0&&fHeld){fHeld=false;if(!fChord)toggle=true;}
   return;
  }
  if(value!=1)return;
  switch(code){
  case KEY_VOLUMEUP:case KEY_VOLUMEDOWN:if(fHeld)fChord=true;break;
  case BTN_THUMBL:case BTN_Z:toggle=true;break; // L3 and HOME: Pause / back
  case BTN_TL:previousLetter=true;break;
  case BTN_TR:nextLetter=true;break;
  case BTN_TL2:swap=-1;break;
  case BTN_TR2:swap=1;break;
  }
 }
 void hat(unsigned axis,int value){
  uint32_t before=hatBits();
  if(axis==ABS_HAT0X)hatX=value;else if(axis==ABS_HAT0Y)hatY=value;else return;
  pressed|=hatBits()&~before;
 }
 void poll(){
  for(auto&d:devices){
   input_event events[64];ssize_t n;bool dropped=false;
   while((n=read(d.fd,events,sizeof events))>0){
    for(size_t i=0;i<size_t(n)/sizeof(input_event);i++){
     auto&e=events[i];
     if(e.type==EV_SYN&&e.code==SYN_DROPPED){dropped=true;continue;}
     if(dropped){if(e.type==EV_SYN&&e.code==SYN_REPORT){resync(d);dropped=false;}continue;}
     if(e.type==EV_KEY)key(e.code,e.value);
     if(e.type==EV_ABS)hat(e.code,e.value);
    }
   }
  }
 }
 uint32_t mask()const{return held|hatBits();}
 uint32_t takePresses(){uint32_t p=pressed;pressed=0;return p;}
 struct Actions {bool toggle=false,previousLetter=false,nextLetter=false;int swap=0;};
 Actions take(){Actions a{toggle,previousLetter,nextLetter,swap};toggle=previousLetter=nextLetter=false;swap=0;return a;}
};

}
