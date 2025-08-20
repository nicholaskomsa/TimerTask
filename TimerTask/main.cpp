#include "TimerTask.h"

int main() {

	using namespace TimerTask;

	//a Timers object uses a background thread to execute timers as coroutines
	Timers timers;

	//(seconds, callback, repeat?=true, repeatCount=forever)
	timers.add(10s, []() { 
		std::println("10");
		return true; 
		});

	timers.add(task(1s, []() { std::println("1"); return true; }, true, 5)
		, task(3s, []() { std::println("3"); return true; }, true, 3)
		, task(6s, []() { std::println("6"); return false; }));


	//main thread logic

    return 0;
}
