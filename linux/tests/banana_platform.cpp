// BananaOS platform layer: the shared dual-panel window's layout, the pad by kernel key codes, and a full UI session
// driven by those codes (library, manufacturer swap, Watch, Pause with F / L3 / HOME, F + volume left to bananad,
// return to the library and exit). Original synthetic pack only: python3 linux/tests/synthetic_pack.py, then
//   MAIN_SOURCE=linux/tests/banana_platform.cpp OUTPUT=/tmp/banana_platform bash linux/build.sh
//   SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /tmp/banana_platform /tmp/yokoi-fixtures/manufacturers.ykp
#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>
static std::atomic<size_t> observedSelection{0};
static std::atomic<int> observedManufacturer{-1};
static std::atomic<bool> observedPlaying{false},observedWatch{false},observedPaused{false};
static std::atomic<int> boots{0};
static std::mutex queueMutex;
static std::deque<std::pair<unsigned,int>> queued;
#define YOKOI_TEST_LIBRARY_SELECTION(s,m,p) do{observedSelection=s;observedManufacturer=m;observedPlaying=p;}while(0)
#define YOKOI_TEST_SESSION(w,p) do{observedWatch=w;observedPaused=p;}while(0)
#define YOKOI_TEST_GAME_CREATED() (++boots)
#define YOKOI_TEST_BANANA_PAD(pad) do{std::lock_guard<std::mutex>lock(queueMutex);while(!queued.empty()){(pad).key(queued.front().first,queued.front().second);queued.pop_front();}}while(0)
#define main yokoi_program
#include "../main.cpp"
#undef main
#include <cassert>
#include <cmath>
#include <filesystem>
#include <thread>

static void send(unsigned code,int value){std::lock_guard<std::mutex>lock(queueMutex);queued.push_back({code,value});}
static void tap(unsigned code){send(code,1);SDL_Delay(70);send(code,0);SDL_Delay(130);}

static void unit(){
 // Layout: BananaOS's SDL puts the top panel left (side) or above (stack).
 setenv("SDL_BANANA_PANELS","side",1);
 auto l=banana::layout("dummy",2048,768);
 assert(l.shared&&l.side&&l.windowW==2048&&l.windowH==768);
 assert(l.panel[0].x==0&&l.panel[0].w==1024&&l.panel[1].x==1024&&l.panel[1].y==0&&l.panel[1].h==768);
 assert(!banana::layout("kmsdrm",2048,768).shared); // only the dummy driver carries the panels
 setenv("SDL_BANANA_PANELS","stack",1);
 l=banana::layout("evdev",640,960);
 assert(l.shared&&!l.side&&l.panel[1].x==0&&l.panel[1].y==480&&l.panel[1].w==640&&l.panel[1].h==480);
 unsetenv("SDL_BANANA_PANELS");
 assert(!banana::layout("dummy",2048,768).shared);
 // A touch normalized over the window, back on its panel.
 setenv("YOKOI_SHARED_WINDOW","side",1);setenv("YOKOI_SHARED_SIZE","1280x480",1);
 l=banana::layout("dummy",1024,768);assert(l.shared&&l.windowW==1280&&l.panel[1].x==640);
 float x=.75f,y=.5f;assert(banana::toPanel(l,x,y)==1&&std::fabs(x-.5f)<1e-6&&std::fabs(y-.5f)<1e-6);
 x=.25f;y=.1f;assert(banana::toPanel(l,x,y)==0&&std::fabs(x-.5f)<1e-6);
 // Pad: the contract's codes, edges for taps, autorepeat ignored.
 banana::Pad pad;pad.simulated=true;assert(pad.available());
 pad.key(BTN_EAST,1);assert(pad.mask()==CTL_A);assert(pad.takePresses()==CTL_A&&pad.takePresses()==0);
 pad.key(BTN_EAST,2);assert(pad.takePresses()==0);pad.key(BTN_EAST,0);assert(pad.mask()==0);
 pad.key(BTN_SOUTH,1);pad.key(BTN_NORTH,1);pad.key(BTN_WEST,1);assert(pad.mask()==(CTL_B|CTL_X|CTL_Y));
 pad.key(BTN_SOUTH,0);pad.key(BTN_NORTH,0);pad.key(BTN_WEST,0);
 pad.key(BTN_START,1);pad.key(BTN_SELECT,1);assert(pad.mask()==(CTL_START|CTL_SELECT));pad.key(BTN_START,0);pad.key(BTN_SELECT,0);
 for(unsigned c:{BTN_DPAD_UP,BTN_DPAD_DOWN,BTN_DPAD_LEFT,BTN_DPAD_RIGHT}){pad.key(c,1);pad.key(c,0);}
 assert(pad.takePresses()==(CTL_B|CTL_X|CTL_Y|CTL_START|CTL_SELECT|CTL_DPAD_UP|CTL_DPAD_DOWN|CTL_DPAD_LEFT|CTL_DPAD_RIGHT));
 pad.hat(ABS_HAT0Y,-1);assert(pad.mask()==CTL_DPAD_UP&&pad.takePresses()==CTL_DPAD_UP);pad.hat(ABS_HAT0Y,0);assert(pad.mask()==0);
 // L1 is the TIME button and the previous initial; R1 only the next initial; L2/R2 swap.
 pad.key(BTN_TL,1);assert(pad.mask()==CTL_L1);pad.key(BTN_TL,0);pad.key(BTN_TR,1);assert(pad.mask()==0);pad.key(BTN_TR,0);
 pad.key(BTN_TR2,1);pad.key(BTN_TR2,0);auto a=pad.take();assert(a.previousLetter&&a.nextLetter&&a.swap==1&&!a.toggle);
 pad.key(BTN_TL2,1);a=pad.take();assert(a.swap==-1);pad.key(BTN_TL2,0);pad.takePresses();
 // F toggles when released; F + volume is bananad's brightness and toggles nothing. L3 and HOME toggle at once.
 pad.key(BTN_MODE,1);assert(!pad.take().toggle);pad.key(BTN_MODE,0);assert(pad.take().toggle);
 pad.key(BTN_MODE,1);pad.key(KEY_VOLUMEUP,1);pad.key(KEY_VOLUMEUP,0);pad.key(BTN_MODE,0);assert(!pad.take().toggle);
 pad.key(BTN_THUMBL,1);assert(pad.take().toggle);pad.key(BTN_THUMBL,0);
 pad.key(BTN_Z,1);assert(pad.take().toggle);pad.key(BTN_Z,0);
 assert(pad.mask()==0&&pad.takePresses()==0);
 // Volume: the mixer is bananad's; the software level stays put.
 RgdsVolume v("/nonexistent/stock","/nonexistent/saved");v.initExternal(10);v.change(-3,10);v.poll(1000);assert(v.level()==10&&std::fabs(v.gain()-.35f)<1e-6);
 unsetenv("YOKOI_SHARED_WINDOW");unsetenv("YOKOI_SHARED_SIZE");
}

int main(int argc,char**argv){
 assert(argc==2);unit();puts("PASS: layout, touch mapping, pad codes, F/L3/HOME, F + volume, fixed volume");
 std::string pack=std::filesystem::absolute(argv[1]);
 char dir[]="/tmp/yokoi-banana-XXXXXX";assert(mkdtemp(dir));assert(chdir(dir)==0);
 setenv("YOKOI_PLATFORM","banana",1);setenv("YOKOI_SHARED_WINDOW","side",1);setenv("YOKOI_SHARED_SIZE","1280x480",1);
 std::thread feeder([]{
  SDL_Delay(400);assert(observedManufacturer==0&&observedSelection==0&&!observedPlaying);
  tap(BTN_TR2);assert(observedManufacturer==1&&observedSelection==2);
  tap(BTN_TL2);assert(observedManufacturer==0&&observedSelection==0);
  tap(BTN_DPAD_DOWN);assert(observedSelection==1);
  tap(BTN_EAST);SDL_Delay(150);assert(observedPlaying&&observedWatch&&boots==1&&!observedPaused);
  tap(BTN_MODE);assert(observedPaused);
  tap(BTN_SOUTH);assert(!observedPaused&&boots==1);
  send(BTN_MODE,1);SDL_Delay(60);send(KEY_VOLUMEUP,1);SDL_Delay(60);send(KEY_VOLUMEUP,0);SDL_Delay(60);send(BTN_MODE,0);SDL_Delay(150);
  assert(!observedPaused);
  tap(BTN_THUMBL);assert(observedPaused);
  tap(BTN_DPAD_DOWN);tap(BTN_EAST);SDL_Delay(150);assert(!observedPlaying&&observedSelection==1&&boots==1);
  tap(BTN_Z);tap(BTN_DPAD_RIGHT);tap(BTN_EAST);
 });
 char name[]="test";char*args[]={name,pack.data(),nullptr};int result=yokoi_program(2,args);feeder.join();
 assert(result==0);assert(chdir("/")==0);std::filesystem::remove_all(dir);
 puts("PASS: BananaOS session: L2/R2 manufacturers, A opens Watch, F/L3 pause, B resumes, F + volume ignored, library return, HOME + A exits");
}
