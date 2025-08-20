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

        using Callback = std::function<bool(void)>;

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

    Task task(std::chrono::milliseconds timeOut, Task::Callback functor, bool repeat = true, std::size_t repeatCount = 0) {

        using Clock = std::chrono::steady_clock;
        auto start = Clock::now();

        int count = 0;
        bool expired = false;

        do {
            auto end = Clock::now();
            auto elapsed = end - start;

            if (elapsed >= timeOut) {
                ++count;
                expired = !functor();
                start = Clock::now();

                if (!repeat || repeat && repeatCount != 0 && count >= repeatCount)
                    expired = true;
            }

            co_yield { count, expired };

        } while (!expired);
    }

    class Timers {
        std::vector<Task> mTasks, newTasks;
        std::mutex newTasksMutex;
        bool mRunning = false;

        void moveNewTasks() {
            std::scoped_lock lock(newTasksMutex);
            if (newTasks.empty()) return;
            auto moveBegin = std::make_move_iterator(newTasks.begin());
            auto moveEnd = std::make_move_iterator(newTasks.end());
            mTasks.insert(mTasks.end(), moveBegin, moveEnd);
            newTasks.clear();
        }

        void run() {
            mRunning = true;
            do {

                moveNewTasks();

                for (auto& t : mTasks)
                    t.resume();

                mTasks.erase(std::remove_if(mTasks.begin(), mTasks.end(),
                    [](auto& t) { return t.expired(); }), mTasks.end());

                std::this_thread::yield();

            } while (!mTasks.empty() && mRunning);
        }

        std::future<void> mTasksFuture;

    public:

        Timers() = default;
        ~Timers() {
            stop();
        }

        void add(std::chrono::milliseconds timeOut, Task::Callback functor, bool repeat = true, std::size_t repeatCount = 0) {
            add(task(timeOut, functor, repeat, repeatCount));
        }
        template<typename... Tasks>
        void add(Task&& task, Tasks&& ...tasks) {

            auto pushTask = [&](auto&& task) {
                newTasks.push_back(std::move(task));
                };

            std::scoped_lock lock(newTasksMutex);

            pushTask(task);
            (pushTask(tasks), ...);

            if (!mTasksFuture.valid() || done())
                mTasksFuture = std::async(std::launch::async, std::bind(&Timers::run, this));
        }

        Timers& operator=(const Timers& other) = delete;

        bool done() {
            return mTasksFuture.valid() && mTasksFuture.wait_for(0s) == std::future_status::ready;
        }

        void stop() {
            mRunning = false;
            if (!done())
                mTasksFuture.get();
        }
    };
}