#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace MySpace
{
    #define THREADPOOL_AUTO_GROW
    class ThreadPool
    {
        using Task = std::function<void()>;
        using Tasks = std::queue<Task>;
        using Threads = std::vector<std::thread>;

    public:
        inline ThreadPool(unsigned int _num)
        {
            AddThread(_num);
        }
        ThreadPool() = delete;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            m_running = false;
            m_task_cv.notify_all();
            for(std::thread& thread : m_pool)
            {
                if(thread.joinable())
                {
                    thread.join();
                }
            }
        }
        template<class F, class ...Args>
        auto CommitTask(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
        {
            if(!m_running)
            {
                throw std::runtime_error("Commit On ThreadPool is Stoppped.");
            }
            
            using RetType = decltype(f(args...));
            auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::future<RetType> tmpfuture = task->get_future();
            {
                std::lock_guard<std::mutex> lock(m_lock);
                m_tasks.emplace([task]{
                    (*task)();
                });
            }
#ifdef THREADPOOL_AUTO_GROW
            if(m_idlThrNum < 1 && m_pool.size() < std::thread::hardware_concurrency())
            {
                AddThread(1);
            }
#endif  // !THREADPOOL_AUTO_GROW
            m_task_cv.notify_one();
            return tmpfuture;
        }

        size_t ThrCount()  {return m_pool.size();}
        int IdlCount()  {return m_idlThrNum;}

    private:
        void AddThread(unsigned int _num)
        {
            for(;m_pool.size() < std::thread::hardware_concurrency() && _num > 0; _num--)
            {
                m_pool.emplace_back([this]{
                    while(m_running)
                    {
                        Task task;
                        {
                            std::unique_lock<std::mutex> lock(m_lock);
                            m_task_cv.wait(lock, [this]{
                                return (!m_running || !m_tasks.empty());
                            });
                            if(!m_running && m_tasks.empty())
                            {
                                return;
                            }
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        m_idlThrNum--;
                        task();
                        m_idlThrNum++;
                    }
                    m_idlThrNum--;                    
                });
            }
        }

    private:
        std::atomic_bool m_running{true};
        std::atomic_int m_idlThrNum{0};
        Tasks m_tasks;
        Threads m_pool;
        std::mutex m_lock;
        std::condition_variable m_task_cv;
    };
};

#endif // !THREAD_POOL_HPP