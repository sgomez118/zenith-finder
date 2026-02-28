#include "logger.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace app {

Logger::Logger() = default;

Logger::~Logger() { Stop(); }

void Logger::Start() {
  if (running_) return;
  running_ = true;
  thread_ = std::thread(&Logger::WriteLoop, this);
}

void Logger::Stop() {
  if (!running_) return;
  running_ = false;
  cv_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Logger::Log(const engine::Observer& obs,
                 const std::vector<engine::CelestialResult>& results) {
  if (!running_) return;

  LogEntry entry;
  entry.time = std::chrono::system_clock::now();
  entry.obs = obs;
  entry.results = results;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(entry));
  }
  cv_.notify_one();
}

void Logger::WriteLoop() {
  std::string filename = GenerateFilename();
  file_.open(filename, std::ios::out);

  if (!file_.is_open()) {
    std::cerr << "Error: Could not open log file " << filename << std::endl;
    running_ = false;
    return;
  }

  // Header
  file_ << "Time,Latitude,Longitude,Altitude,Star,Elevation,Azimuth,ZenithDist"
        << std::endl;

  while (running_ || !queue_.empty()) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&] { return !queue_.empty() || !running_; });

    while (!queue_.empty()) {
      LogEntry entry = std::move(queue_.front());
      queue_.pop();
      lock.unlock();

      auto time_t = std::chrono::system_clock::to_time_t(entry.time);
      std::tm tm_now;
      gmtime_s(&tm_now, &time_t);

      std::string time_str = std::format(
          "{:04}-{:02}-{:02} {:02}:{:02}:{:02}", tm_now.tm_year + 1900,
          tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min,
          tm_now.tm_sec);

      for (const auto& res : entry.results) {
        file_ << std::format("{},{:.6f},{:.6f},{:.2f},{},{:.4f},{:.4f},{:.4f}\n",
                             time_str, entry.obs.latitude, entry.obs.longitude,
                             entry.obs.altitude, res.name, res.elevation,
                             res.azimuth, res.zenith_dist);
      }
      file_.flush();

      lock.lock();
    }
  }

  file_.close();
}

std::string Logger::GenerateFilename() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm_now;
  gmtime_s(&tm_now, &time_t);

  return std::format("zenith_log_{:04}{:02}{:02}_{:02}{:02}{:02}.csv",
                     tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                     tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
}

}  // namespace app
