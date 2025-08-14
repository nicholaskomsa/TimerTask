#include "TimerTask.h"

int main() {

	using namespace TimerTask;

	Timers timers;

	timers.addGroup(task(1s, []() { std::print("Timer 1 executed\n"); return true; }, true, 5)
		, task(2s, []() { std::print("Timer 2 executed\n"); return true; }, true, 3)
		, task(3s, []() { std::print("Timer 3 executed\n"); return false; }));

	timers.add(10s, []() { std::print("Timer 4 executed\n"); return true; }, true, 0);


    return 0;
}
