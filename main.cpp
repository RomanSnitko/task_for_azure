#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

/* Ниже код логгера. Он должен:

быть доступен глобально через Logger::instance();
безопасно работать из нескольких потоков;
присваивать каждому потоку свой thread_id;
считать общее количество сообщений;
корректно завершаться;
не падать при уничтожении глобальных/static объектов. */

/* Исправьте программу так, чтобы она была корректной согласно C++ memory model, 
не содержала data races, не зависела от порядка уничтожения static/global объектов 
и при этом сохранила идею function-local static и thread-local ID.*/

class Logger
{
public:
    static Logger& instance()
    {
        static Logger logger;
        return logger;
    }

    int& threadId()
    {
        static thread_local int id = registerThread();
        return id;
    }

    void log(const std::string& message)
    {
        ++messageCount_;

        const int id = threadId();

        messages_[id].push_back(message);

        std::cout
            << "[thread " << id << "] "
            << message
            << '\n';
    }

    const std::vector<std::string>& messagesForCurrentThread()
    {
        return messages_[threadId()];
    }

    std::size_t messageCount() const
    {
        return messageCount_;
    }

    ~Logger()
    {
        std::cout << "Logger destroyed\n";

        for (const auto& [id, messages] : messages_)
        {
            std::cout
                << "Thread " << id
                << ": " << messages.size()
                << " messages\n";
        }
    }

private:
    Logger() = default;

    int registerThread()
    {
        return nextThreadId_++;
    }

private:
    static inline int nextThreadId_ = 0;

    std::size_t messageCount_ = 0;

    std::unordered_map<
        int,
        std::vector<std::string>
    > messages_;
};

class GlobalWorker
{
public:
    ~GlobalWorker()
    {
        Logger::instance().log(
            "GlobalWorker destroyed"
        );
    }
};

GlobalWorker globalWorker;

void worker()
{
    for (int i = 0; i < 1000; ++i)
    {
        Logger::instance().log("hello");
    }

    const auto& messages =
        Logger::instance()
            .messagesForCurrentThread();

    std::cout
        << "thread stored "
        << messages.size()
        << " messages\n";
}

int main()
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back(worker);
    }

    for (auto& t : threads)
        t.join();

    std::cout
        << "total: "
        << Logger::instance().messageCount()
        << '\n';
}