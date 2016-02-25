/******************************************************************************
    Copyright © 2012-2015 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "runtime/Thread.h"
#include "kernel/AddressSpace.h"
#include "kernel/Clock.h"
#include "kernel/Output.h"
#include "world/Access.h"
#include "machine/Machine.h"
#include "devices/Keyboard.h"

#include "main/UserMain.h"

AddressSpace kernelSpace(true); // AddressSpace.h
volatile mword Clock::tick;     // Clock.h

extern Keyboard keyboard;

#if TESTING_KEYCODE_LOOP
static void keybLoop() {
  for (;;) {
    Keyboard::KeyCode c = keyboard.read();
    StdErr.print(' ', FmtHex(c));
  }
}
#endif

void kosMain() {
  KOUT::outl("Welcome to KOS!", kendl);
  auto iter = kernelFS.find("motb");
  if (iter == kernelFS.end()) {
    KOUT::outl("motb information not found");
  } else {
    FileAccess f(iter->second);
    for (;;) {
      char c;
      if (f.read(&c, 1) == 0) break;
      KOUT::out1(c);
    }
    KOUT::outl();
  }
  
  iter = kernelFS.find("schedParams");
  if (iter == kernelFS.end()) {
    KOUT::outl("schedParams information not found!");
	Scheduler::defEpoch = 50000000;
	Scheduler::minGran = 5000000;
  } else {
    FileAccess f(iter->second);
	int paramCounter = 0;
    for (;;) {
	  char c;
	  char val[5];
	  if (f.read(&c, 1) == 0) break;  
	  while (c != '*') { // read until a * is found or until end of file
		 if (f.read(&c, 1) == 0) break;
	     continue;	  
	  }
	  
	  if (f.read(&c, 1) == 0) break;  
	  int length = 1;
	  val[4] = c;
	  for (int i = 3; i >= 0; i--){ // save chars until next *
		f.read(&c, 1);
		if (c == 42) break;
		val[i] = c;
		length++;
	  }

	  int total = 0;
	  for (int i = 0; i < 5; i++){ //convert chars to int
		  if (val[i] == '\0' || val[i] == '\n') 
		  {
		    continue;
		  }
		  int num = val[i] - 48;
		  int multiple = 1;
		  for (int j = (i - (5-length)); j > 0; j--){
			multiple *= 10;
		  }
		  total += num * multiple;
	  }
	  KOUT::out1("total = ", total);
	  if (paramCounter == 0){
		Scheduler::minGran = (Machine::freq*total)/1000;
		KOUT::out1("Minimum Granularity (ms) = ", total);
		KOUT::outl();
		KOUT::out1("Minimum Granularity (cycles) = ", Scheduler::minGran);
		KOUT::outl();
	  }else if (paramCounter == 1){
		Scheduler::defEpoch = (Machine::freq*total)/1000;
		KOUT::out1("Default Epoch (ms) = ", total);
		KOUT::outl();
		KOUT::out1("Default Epoch (cycles) = ", Scheduler::defEpoch);
		KOUT::outl();
	  }
	  paramCounter++;	  
    }
    KOUT::outl();
  }
#if TESTING_TIMER_TEST
  StdErr.print(" timer test, 3 secs...");
  for (int i = 0; i < 3; i++) {
    Timeout::sleep(Clock::now() + 1000);
    StdErr.print(' ', i+1);
  }
  StdErr.print(" done.", kendl);
#endif
#if TESTING_KEYCODE_LOOP
  Thread* t = Thread::create()->setPriority(topPriority);
  Machine::setAffinity(*t, 0);
  t->start((ptr_t)keybLoop);
#endif
  Thread::create()->start((ptr_t)UserMain);
#if TESTING_PING_LOOP
  for (;;) {
    Timeout::sleep(Clock::now() + 1000);
    KOUT::outl("...ping...");
  }
#endif
}

extern "C" void kmain(mword magic, mword addr, mword idx)         __section(".boot.text");
extern "C" void kmain(mword magic, mword addr, mword idx) {
  if (magic == 0 && addr == 0xE85250D6) {
    // low-level machine-dependent initialization on AP
    Machine::initAP(idx);
  } else {
    // low-level machine-dependent initialization on BSP -> starts kosMain
    Machine::initBSP(magic, addr, idx);
  }
}
