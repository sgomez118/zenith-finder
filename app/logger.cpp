#include "logger.hpp"

#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace app {

Logger::Logger() {}

Logger::~Logger() { Stop(); }

void Logger::Start() {
  if (running_) return;

  std::string filename = GenerateFilename();
  file_.open(filename, std::ios::out);
  if (!file_.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
    return;
  }

  // Write Header
  file_ << "Timestamp,Lat,Lon,Alt,Star,Elevation,Azimuth,ZenithDist,Status\n";

  running_ = true;
  thread_ = std::thread(&Logger::WriteLoop, this);
}

void Logger::Stop() {
  running_ = false;
  cv_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
  if (file_.is_open()) {
    file_.close();
  }
}

void Logger::Log(const engine::Observer& obs,
                 const std::vector<engine::CelestialResult>& results) {
  if (!running_) return;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push({std::chrono::system_clock::now(), obs, results});
  }
  cv_.notify_one();
}

void Logger::WriteLoop() {
  while (running_ || !queue_.empty()) {
    LogEntry entry;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

      if (queue_.empty() && !running_) break;

      entry = std::move(queue_.front());
      queue_.pop();
    }

    auto time_t = std::chrono::system_clock::to_time_t(entry.time);
    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

    for (const auto& res : entry.results) {
      file_ << std::format(
          "{},{:.4f},{:.4f},{:.1f},{},{:.2f},{:.2f},{:.2f},{}\n", ss.str(),
          entry.obs.latitude, entry.obs.longitude, entry.obs.altitude, res.name,
          res.elevation, res.azimuth, res.zenith_dist,
          (res.is_rising ? "RISING" : "SETTING"));
    }
    file_.flush();
  }
}

std::string Logger::GenerateFilename() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf;
#ifdef _WIN32
  gmtime_s(&tm_buf, &time_t);
#else
  gmtime_r(&time_t, &tm_buf);
#endif
  std::stringstream ss;
  ss << "zenith_log_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".csv";
  return ss.str();
}

}  // namespace app
