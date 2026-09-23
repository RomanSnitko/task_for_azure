#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <syncstream>

/* Below is the logger code. It must:

be globally accessible via Logger::Instance();
be safe to run across multiple threads;
assign each thread its own thread_id;
count the total number of messages;
terminate gracefully;
not crash when global/static objects are destroyed. */

/* Fix the program so that it complies with the C++ memory model,
does not contain data races, is not dependent on the order of static/global object destruction,
and still preserves the concept of function-local static and thread-local IDs.*/

class Logger
{
public:
    static Logger& Instance()
    {
        static Logger* logger = new Logger; // dtor not autocalled

        //static Logger logger; // init thread-safety
        return *logger;
    }

    int GetThreadId()
    {
        /* expected fucking idea:
            next_thread_id = 0;
            threadA: 
                return 0; next_thread_id == 1;
            threadB:
                return 1; next_thread_id == 2;
            threadC:
                return 2; ...
            threadD:
                return 3; ...
        */
        static thread_local int id = RegisterThread();
        return id;
    }

    void Log(const std::string& message)
    {
        std::osyncstream synced_out(std::cout);

        message_count.fetch_add(1, std::memory_order_relaxed);

        const int id = GetThreadId();

        {
            std::lock_guard lock(messages_mutex);
            messages[id].push_back(message);
        }

        synced_out << "[thread " << id << "] " << message << '\n';
    }

    std::vector<std::string> MessagesForCurrentThread()
    {
        std::lock_guard lock(messages_mutex);
        return messages[GetThreadId()];
    }

    std::size_t GetMessageCount() const
    {
        message_count.load(std::memory_order_relaxed);
        // return message_count;
    }

    ~Logger()
    {
        std::cout << "Logger destroyed\n";

        for (const auto& [id, messages] : messages)
        {
            std::cout << "Thread " << id << ": " << messages.size() << " messages\n";
        }
    }

private:
    Logger() = default;

    int RegisterThread()
    {
        return next_thread_id.fetch_add(1, std::memory_order_relaxed);
        // need to unique and atomic increment
        // return next_thread_id++;
    }

private:
    static inline std::atomic<int> next_thread_id{0};
    // static inline int next_thread_id = 0;

    std::atomic<std::size_t> message_count{0};
    // std::size_t message_count = 0;

    std::mutex messages_mutex;

    std::unordered_map<int, std::vector<std::string>> messages;
};


class GlobalWorker
{
public:
    ~GlobalWorker()
    {
        Logger::Instance().Log("GlobalWorker destroyed");
    }
};

GlobalWorker global_worker;


void worker()
{
    for (int i = 0; i < 1000; ++i)
    {
        Logger::Instance().Log("hello");
    }

    const auto& messages = Logger::Instance().MessagesForCurrentThread();

    std::cout << "thread stored " << messages.size() << " messages\n";
}


int main()
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([]()
        {
            worker();
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    std::cout << "total: " << Logger::Instance().GetMessageCount() << '\n';

    /* problem lifetime Logger:
    1. global_worker is created
    2. main() start
    3. Logger::Instance()
    3. logger is created

    ~GlobalWOrker contains a Logger::Instance()
    */
}