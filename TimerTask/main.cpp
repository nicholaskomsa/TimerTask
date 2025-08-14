#include "TimerTask.h"

int main() {

	using namespace TimerTask;

	Timers timers;
	timers.add(task(1s, []() { std::print("Timer 1 executed\n"); }, true, 5)
		, task(2s, []() { std::print("Timer 2 executed\n"); }, true, 3)
		, task(3s, []() { std::print("Timer 3 executed\n"); }, false));

    return 0;
}
