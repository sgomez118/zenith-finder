#ifndef APP_LOGGER_HPP_
#define APP_LOGGER_HPP_

#include "engine.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace app {

class Logger {
public:
    Logger();
    ~Logger();

    void Log(const engine::Observer& obs, const std::vector<engine::CelestialResult>& results);
    void Start();
    void Stop();

private:
    struct LogEntry {
        std::chrono::system_clock::time_point time;
        engine::Observer obs;
        std::vector<engine::CelestialResult> results;
    };

    void WriteLoop();
    std::string GenerateFilename();

    std::ofstream file_;
    std::queue<LogEntry> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace app

#endif // APP_LOGGER_HPP_
