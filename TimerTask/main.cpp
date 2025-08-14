#include "TimerTask.h"

int main() {

	using namespace TimerTask;

	//a Timers object uses a background thread to execute timers as coroutines
	Timers timers;

	//(seconds, callback, repeat?, repeatCount=forever)
	timers.add(10s, []() { 
		std::println("Ten");
		return true; 
		});

	timers.addGroup(task(1s, []() { std::println("One"); return true; }, true, 5)
		, task(2s, []() { std::println("Two"); return true; }, true, 3)
		, task(3s, []() { std::println("Three"); return false; }));

	//main thread logic

    return 0;
}
