#include <coroutine>
#include <format>
#include <print>
#include <functional>
#include <chrono>
#include <future>

using namespace std::chrono_literals;

struct TimerTask {

    struct YieldType {
        int mCount = 0;
        bool mExpired = false;
    };

    struct promise_type {

        using Coroutine = std::coroutine_handle<promise_type>;

        YieldType mCurrentValue;

        TimerTask get_return_object() { return { Coroutine::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }

        std::suspend_always yield_value(const YieldType& value) {
            mCurrentValue = value;
            return {};
        }
    };

    using Coroutine = promise_type::Coroutine;
    Coroutine mCoro;

    using Task = std::function<void(void)>;

    TimerTask(Coroutine h) : mCoro(h) {}
    ~TimerTask() { if (mCoro) mCoro.destroy(); }

    TimerTask(TimerTask&& other) noexcept : mCoro(other.mCoro) {
        other.mCoro = nullptr;
    }

    TimerTask& operator=(TimerTask&& other) noexcept {
        if (this != &other) {
            if (mCoro) mCoro.destroy();
            mCoro = other.mCoro;
            other.mCoro = nullptr;
        }
        return *this;
    }

    bool resume() {
        if (!mCoro.done())
            mCoro.resume();
        return !mCoro.done();
    }

    YieldType value() const { return mCoro.promise().mCurrentValue; }
    bool expired() const { return value().mExpired; }
};

TimerTask timerTask(std::chrono::seconds timeOut, TimerTask::Task functor, bool repeat = true, std::size_t repeatCount = 0) {

    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    int count = 0;
    bool expired = false;

    do {
        auto end = Clock::now();
        auto elapsed = end - start;

        if (elapsed >= timeOut) {
            ++count;
            functor();
            start = Clock::now();
            if (!repeat || repeat && count >= repeatCount) {
                expired = true;
                break;
            }
        }
        co_yield{ count, expired };

    } while (true);

    co_yield{ count, expired };
}

int main() {

    using Timers = std::vector<TimerTask>;
    Timers timers;

    auto setupTimers = [&]() {

        auto printOne = timerTask(1s, [&] {
            std::println("one");
            }, true, 10);

        auto printFive = timerTask(5s, [&] {
            std::println("five");
            }, true, 3);

        auto printTen = timerTask(10s, [&] {
            std::println("ten");
            }, false);

        timers.push_back(std::move(printOne));
        timers.push_back(std::move(printFive));
        timers.push_back(std::move(printTen));
        };

    setupTimers();

    auto timersFuture = std::async(std::launch::async, [&] {

        do {
            for (auto& t : timers)
                t.resume();

            timers.erase(std::remove_if(timers.begin(), timers.end(),
                [](auto& t) { return t.expired(); }), timers.end());

        } while (!timers.empty());

        });

    timersFuture.get();

    return 0;
}
