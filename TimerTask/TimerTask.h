#include <coroutine>
#include <format>
#include <print>
#include <functional>
#include <chrono>
#include <future>

namespace TimerTask {

    using namespace std::chrono_literals;

    struct Task {

        struct YieldType {
            int mCount = 0;
            bool mExpired = false;
        };

        struct promise_type {

            using Coroutine = std::coroutine_handle<promise_type>;

            YieldType mCurrentValue;

            Task get_return_object() { return { Coroutine::from_promise(*this) }; }
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

        using Callback = std::function<void(void)>;

        Task(Coroutine h) : mCoro(h) {}
        ~Task() { if (mCoro) mCoro.destroy(); }

        Task(Task&& other) noexcept : mCoro(other.mCoro) {
            other.mCoro = nullptr;
        }

        Task& operator=(Task&& other) noexcept {
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

    Task task(std::chrono::seconds timeOut, Task::Callback functor, bool repeat = true, std::size_t repeatCount = 0) {

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

    class Timers {

        std::vector<Task> mTasks, newTasks;
        std::mutex newTimersMutex;
        bool mRunning = false;

        void moveNewTimers() {
            std::scoped_lock lock(newTimersMutex);
            auto moveBegin = std::make_move_iterator(newTasks.begin());
            auto moveEnd = std::make_move_iterator(newTasks.end());
            mTasks.insert(mTasks.end(), moveBegin, moveEnd);
            newTasks.clear();
        }

        void run() {
            mRunning = true;
            do {

                moveNewTimers();

                for (auto& t : mTasks)
                    t.resume();

                mTasks.erase(std::remove_if(mTasks.begin(), mTasks.end(),
                    [](auto& t) { return t.expired(); }), mTasks.end());

                std::this_thread::yield();

            } while (!mTasks.empty() && mRunning);
        }

        std::future<void> mTimersFuture;

    public:

        Timers() = default;

        template<typename... Args>
        void add(Args&& ...args) {

            auto pushArg = [&](auto& arg) {
                newTasks.push_back(std::move(arg));
                };

            std::scoped_lock lock(newTimersMutex);

            (pushArg(args), ...);

            if (!mTimersFuture.valid() || done())
                mTimersFuture = std::async(std::launch::async, std::bind(&Timers::run, this));
        }

        Timers& operator=(const Timers& other) = delete;

        bool done() {
            return mTimersFuture.valid() && mTimersFuture.wait_for(0s) == std::future_status::ready;
        }

        void stop() {
            mRunning = false;
            if (mTimersFuture.valid())
                mTimersFuture.get();
        }
    };
}