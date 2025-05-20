#pragma once

#include <any>
#include <thread>
#include <future>
#include "Common.h"
#include "LogLib.h"

#include <unordered_map>
#include <queue>

struct BaseThreadContext
{
    std::chrono::time_point<std::chrono::steady_clock> start_point;
    std::thread             thread;
    std::string             name;
};

struct ThreadContext : BaseThreadContext
{
    std::condition_variable conditional;
    std::mutex              mutex;
    std::any                result;
};

class ThreadPool
{
public:
    enum class CallableType
    {
        Undecided,
        Voidable,
        ResultAny
    };

private:
    struct TaskResult
    {
        bool                    finished = false;
        const CallableType      type = CallableType::Undecided;
        std::any                result;

        TaskResult(CallableType _type) : type(_type) {}
    };

    class Task
    {
    private:
        static void EmptyVoidPlaceHolder() {}
        static int EmptyIntPlaceHolder() { return 0; }

        TaskResult                  m_Result;
        std::string                 m_TaskName;
        std::function<void()>       m_VoidFoo = EmptyVoidPlaceHolder;
        std::function<std::any()>   m_AnyFoo = EmptyIntPlaceHolder;

        Task() : m_Result(CallableType::Undecided) {}
    public:
        static Task GetEmptyTask()
        {
            static Task empty_task;
            return empty_task;
        }

        Task(const std::string &taskName, std::function<void()> voidFunction) :
            m_Result(CallableType::Voidable), m_TaskName(taskName), m_VoidFoo(voidFunction) {}

        Task(const std::string &taskName, std::function<std::any()> anyFunction) :
            m_Result(CallableType::ResultAny), m_TaskName(taskName), m_AnyFoo(anyFunction) {}

        void operator() ()
        {
            const auto tast_start = std::chrono::steady_clock::now();
            if (m_Result.type == CallableType::Voidable)
            {
                m_VoidFoo();
            }
            else if (m_Result.type == CallableType::ResultAny)
            {
                m_Result.result = m_AnyFoo();
            }
            Log_DebugF("Task {} time execution {}.", m_TaskName, std::chrono::steady_clock::now() - tast_start);
            m_Result.finished = true;
        }

        TaskResult get_result() const
        {
            return m_Result;
        }

        template <typename ...CallableT, typename ...ArgT>
        static std::function<void()> WarpCallable(void(&&func)(CallableT...), ArgT &&...args)
        {
            return std::bind(std::forward<decltype(func)>(func), std::forward<ArgT>(args)...);
        }
        template <typename CallableR, typename ...CallableT, typename ...ArgT>
        static std::function<std::any()> WarpCallable(CallableR(&&func)(CallableT...), ArgT &&...args)
        {
            return std::bind(std::forward<decltype(func)>(func), std::forward<ArgT>(args)...);
        }
    };

    struct PoolWorker
    {
        static constexpr auto defaultAwaitTime = std::chrono::seconds(10);

        enum class Priority
        {
            Common,
            Exclusive
        };
        enum class State : uint8_t
        {
            Created,        // After init
            Started,        // Before first execution
            Working,        // Processing tasks
            Await,          // Waiting new task
            Stopped,        // No more executions
            PostExecution   // Await dstr call
        };

        Priority            priority = Priority::Common;
        std::atomic<State>  state = State::Created;
        std::chrono::nanoseconds awaitTime = defaultAwaitTime;
        ThreadPool          &parent;
        ThreadContext        context;

        PoolWorker(ThreadPool &parent, const std::string &taskName) :
            parent(parent), awaitTime(defaultAwaitTime)
        {
            context.name = taskName;
            context.thread = std::thread(&PoolWorker::WorkerLoop, this);
        }

        ~PoolWorker()
        {
            state = State::PostExecution;
            context.conditional.notify_one();
            if (context.thread.joinable())
            {
                context.thread.join();
            }
        }

        const State GetState() const { return state.load(); }

        bool IsTaskDead() const { const auto current_state = GetState();  return current_state == State::Stopped || current_state == State::PostExecution; }

        bool IsTaskActive() const { const auto current_state = GetState();  return current_state == State::Working || current_state == State::Await; }

        void Signal(const State newState)
        {
            ChangeActiveState(newState);
            context.conditional.notify_one();
        }

    private:
        void ChangeActiveState(const State newState)
        {
            if (!IsTaskDead())
            {
                state.store(newState);
            }
        }

        void WorkerLoop()
        {
            std::unique_lock lock(context.mutex);
            static const auto empty_task_data = std::pair<uint64_t, Task>(0, Task::GetEmptyTask());
            state.store(State::Started);
            while (state < State::Stopped)
            {
                ChangeActiveState(State::Await);
                auto working_ctx = parent.PullTask(*this);
                if (!working_ctx.second)
                {
                    state = State::Stopped;
                    continue;
                }
                if (working_ctx.second)
                {
                    ChangeActiveState(State::Working);
                    auto callable = working_ctx.second.get();
                    (*callable)();
                    parent.AddResult(working_ctx.first, std::move(working_ctx.second->get_result()));
                }
            }
        }
    };
    friend struct PoolWorker;

    std::mutex                                                  m_PullMutex;
    std::condition_variable_any                                 m_ResultCV;

    size_t                                                      m_MaxWorkers;
    uint64_t                                                    m_TaskIdx = 0;
    std::recursive_mutex                                        m_RequestMutex;
    std::queue<std::pair<uint64_t, std::shared_ptr<Task>>>      m_QueuedTasks;      // Any task`ll be placed here before execution.

    std::unordered_map<uint64_t, std::any>                      m_ResultKeeper;
    std::unordered_map<size_t, std::shared_ptr<PoolWorker>>     m_StartedWorkers;   // Container of an active threads.

    std::pair<uint64_t, std::shared_ptr<Task>> PullTask(PoolWorker &worker)
    {
        std::unique_lock lock(m_PullMutex);
        std::cv_status wait_status = std::cv_status::no_timeout;
        do
        {
            if (m_QueuedTasks.empty() && wait_status == std::cv_status::no_timeout)
            {
                wait_status = worker.context.conditional.wait_for(lock, worker.awaitTime);
                continue;
            }
        } while (false);

        if (m_QueuedTasks.empty() || worker.IsTaskDead())
        {
            return {};
        }
        auto front = m_QueuedTasks.front();
        m_QueuedTasks.pop();
        return front;
    }

    void AddResult(uint64_t taskIdx, TaskResult &&result)
    {
        m_ResultKeeper[taskIdx] = std::move(result);
        m_ResultCV.notify_all();
    }

    // We need to make sure we have at least 1 thread await or we have to create new, because old one dies.
    void SignalOrAddMorWorker()
    {
        for (size_t i = 0; i < m_MaxWorkers; i++)
        {
            const auto *i_worker = m_StartedWorkers[i].get();
            if (i_worker && i_worker->GetState() == PoolWorker::State::Await)
            {
                break;
            }
            else if (!i_worker || i_worker->IsTaskDead())
            {
                std::stringstream ss; ss << "Worker " << i;
                m_StartedWorkers[i] = std::make_shared<PoolWorker>(*this, ss.str());
                break;
            }
        }
    }

public:
    static ThreadPool *GLobalInstance()
    {
        static ThreadPool instance;
        return &instance;
    }

    ThreadPool(uint32_t workersSize = 0)
    {
        if (workersSize == 0)
        {
            workersSize = std::thread::hardware_concurrency();
        }
        m_MaxWorkers = workersSize;
    }

    ThreadPool(ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &) = delete;
    ~ThreadPool()
    {
        std::unique_lock lock(m_RequestMutex);
        WaitAllTasks();
        for (auto &[id, worker] : m_StartedWorkers)
        {
            worker.reset();
        }
    }

    template <typename CallableR, typename ...CallableT, typename ...ArgT>
    uint64_t AddTask(const std::string &taskName, CallableR(&&func)(CallableT...), ArgT &&...args)
    {
        std::lock_guard lock(m_RequestMutex);
        const auto task_id = ++m_TaskIdx;
        m_QueuedTasks.push({ task_id, std::make_shared<Task>(taskName, Task::WarpCallable(std::forward<decltype(func)>(func), std::forward<ArgT>(args)...)) });
        SignalOrAddMorWorker();
        return task_id;
    }

    void ClearQueue()
    {
        std::lock_guard lock_request(m_RequestMutex);
        std::lock_guard lock_pull(m_PullMutex);
        m_QueuedTasks.swap(std::queue<std::pair<uint64_t, std::shared_ptr<Task>>>());
    }

    void WaitTask(const uint64_t taskId)
    {
        std::mutex mux;
        std::unique_lock lock(mux);
        const auto pred = [=]()
        {
            return m_ResultKeeper.find(taskId) != m_ResultKeeper.end();
        };
        if (!pred())
        {
            m_ResultCV.wait(lock, pred);
        }
    }

    void WaitAllTasks()
    {
        std::unique_lock lock(m_RequestMutex);
        const uint64_t last_task_id = m_TaskIdx;
        const auto pred = [=]()
        {
            return m_ResultKeeper.size() == last_task_id;
        };
        if (!pred())
        {
            m_ResultCV.wait(lock, pred);
        }
    }
};