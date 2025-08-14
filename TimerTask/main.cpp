#include "TimerTask.h"

int main() {

	using namespace TimerTask;

	Timers timers;

	timers.add(10s, [](){ std::println("Ten"); return true; });

	timers.addGroup(task(1s, []() { std::println("One"); return true; }, true, 5)
		, task(2s, []() { std::println("Two"); return true; }, true, 3)
		, task(3s, []() { std::println("Three"); return false; }));

    return 0;
}
