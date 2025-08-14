#include "HttpTelemetry.hpp"
#include "Application.hpp"
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <chrono>
#include <ctime>
#include <string>

namespace {
std::string
iso8601formatUTC(std::chrono::system_clock::time_point utctimept) {
  auto utc = std::chrono::system_clock::to_time_t(utctimept);
  struct tm tm;
  gmtime_r(&utc, &tm);
  constexpr char format[] = "%Y-%m-%dT%H:%M:%SZ";
  constexpr std::size_t SIZE = std::size(format) * 2;
  std::string buffer(SIZE, '\0');
  std::size_t len = std::strftime(buffer.data(), SIZE, format, &tm);
  buffer.resize(len);
  return buffer;
}
} // namespace

//
//
//
HttpTelemetry::HttpTelemetry() {}

//
//
//
void HttpTelemetry::task_handler() {
  auto data = Application::getElectricPowerData();

  if (!data.instant_watt) {
    return;
  }

  if (!_previous_instant_watt) {
    _previous_instant_watt = data.instant_watt;
    return;
  }

  if (_previous_instant_watt->second.watt == data.instant_watt->second.watt) {
    // same value, do nothing
    return;
  }

  // A change was detected. Log the previous value and the duration.
  JsonDocument doc;
  doc["instant_watt"] = _previous_instant_watt->second.watt.count();
  doc["measured_at"] = iso8601formatUTC(_previous_instant_watt->first);
  auto diff = data.instant_watt->first - _previous_instant_watt->first;
  doc["duration_ms"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

  std::string output;
  serializeJson(doc, output);
  M5_LOGI("HttpTelemetry data: %s", output.c_str());

  // Update the previous value
  _previous_instant_watt = data.instant_watt;
}
