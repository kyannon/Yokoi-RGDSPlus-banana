#pragma once
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
struct Gesture {int swipe=0;bool tap=false,doubled=false;float x=0,y=0;int panel=1;};
class TouchGesture {
 bool held=false;float sx=0,sy=0,lastX=0,lastY=0;uint64_t start=0,lastTap=0;
public:
 void down(float x,float y,uint64_t now){if(held)return;held=true;sx=x;sy=y;start=now;}
 Gesture up(float x,float y,uint64_t now,int panel){Gesture g{};g.x=x;g.y=y;g.panel=panel;if(!held)return g;held=false;
  float dx=x-sx,dy=y-sy;if(std::abs(dx)>.13f&&std::abs(dx)>std::abs(dy)*1.2f){g.swipe=dx<0?1:-1;lastTap=0;}
  else if(now-start<650&&std::abs(dx)<.08f&&std::abs(dy)<.08f){g.tap=true;g.doubled=lastTap&&now-lastTap<380&&std::abs(x-lastX)<.09&&std::abs(y-lastY)<.09;lastTap=g.doubled?0:now;lastX=x;lastY=y;}return g;
 }
};
class RawTouch {
 int fd=-1;input_absinfo ax{},ay{};int x=0,y=0;bool down=false,previous=false;TouchGesture gesture;
public:
 ~RawTouch(){if(fd>=0)close(fd);}
 bool available()const{return fd>=0;}
 // Stock OS: the gt9xx touchscreen. sysfsMatch (BananaOS): the device whose sysfs path contains it, any name.
 void openDevice(const char*sysfsMatch=nullptr){glob_t paths{};if(glob("/dev/input/event*",0,nullptr,&paths)){globfree(&paths);return;}
  for(size_t i=0;i<paths.gl_pathc;i++){
   bool wanted=true;
   if(sysfsMatch&&*sysfsMatch){std::string node=paths.gl_pathv[i],link="/sys/class/input/"+node.substr(node.find_last_of('/')+1);char real[4096];wanted=realpath(link.c_str(),real)&&strstr(real,sysfsMatch);}
   if(!wanted)continue;
   int f=open(paths.gl_pathv[i],O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(f<0)continue;char name[256]{};ioctl(f,EVIOCGNAME(sizeof name),name);
   if((sysfsMatch&&*sysfsMatch?true:strstr(name,"gt9xx")!=nullptr)&&ioctl(f,EVIOCGABS(ABS_MT_POSITION_X),&ax)==0&&ioctl(f,EVIOCGABS(ABS_MT_POSITION_Y),&ay)==0&&ax.maximum>ax.minimum&&ay.maximum>ay.minimum){fd=f;printf("RGDS touch: %s (%s), %d..%d x %d..%d\n",paths.gl_pathv[i],name,ax.minimum,ax.maximum,ay.minimum,ay.maximum);break;}close(f);
  }globfree(&paths);
 }
 std::vector<Gesture>poll(uint64_t now){std::vector<Gesture>out;if(fd<0)return out;input_event e[32];ssize_t n;while((n=read(fd,e,sizeof e))>0)for(size_t i=0;i<size_t(n)/sizeof(input_event);i++){
   auto&v=e[i];if(v.type==EV_ABS){if(v.code==ABS_MT_POSITION_X)x=v.value;if(v.code==ABS_MT_POSITION_Y)y=v.value;if(v.code==ABS_MT_TRACKING_ID)down=v.value>=0;}
   if(v.type==EV_KEY&&v.code==BTN_TOUCH)down=v.value!=0;
   if(v.type==EV_SYN&&v.code==SYN_REPORT){float nx=std::clamp((x-ax.minimum)/float(std::max(1,ax.maximum-ax.minimum)),0.f,1.f),ny=std::clamp((y-ay.minimum)/float(std::max(1,ay.maximum-ay.minimum)),0.f,1.f);
    if(down&&!previous)gesture.down(nx,ny,now);if(!down&&previous)out.push_back(gesture.up(nx,ny,now,1));previous=down;}
  }return out;
 }
};
