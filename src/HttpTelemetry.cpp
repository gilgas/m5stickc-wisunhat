#include "HttpTelemetry.hpp"
#include "Application.hpp"
#include "EchonetLite.hpp"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <chrono>
#include <ctime>
#include <string>

namespace {
std::string iso8601formatUTC(std::chrono::system_clock::time_point utctimept) {
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

void send_json(const std::string &endpoint, const JsonDocument &doc) {
  JsonDocument send_data;
  send_data["action"] = "replace";
  send_data["sheetName"] = "b-route";
  send_data["rows"] = JsonArray();
  send_data["rows"].add(doc);
  std::string output;
  serializeJson(send_data, output);
  M5_LOGD("HttpTelemetry data: %s", output.c_str());

  if (endpoint.empty()) {
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // For testing, use a proper certificate in production
  HTTPClient http;
  http.begin(client, endpoint.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  int httpCode = http.POST(output.c_str());
  if (httpCode > 0) {
    M5_LOGD("HTTP POST successful, code: %d", httpCode);
    if (httpCode >= 400) {
      String payload = http.getString();
      M5_LOGW("HTTP response: %s", payload.c_str());
    }
  } else {
    M5_LOGE("HTTP POST failed, error: %s",
            http.errorToString(httpCode).c_str());
  }
  http.end();
}
} // namespace

//
//
//
HttpTelemetry::HttpTelemetry(std::string endpoint)
    : _endpoint{std::move(endpoint)} {}

//
//
//
void HttpTelemetry::task_handler() {
  auto data = Application::getElectricPowerData();
  bool update_required = false;

  // InstantWatt
  if (data.instant_watt) {
    if (!_previous_instant_watt || (_previous_instant_watt->second.watt !=
                                    data.instant_watt->second.watt)) {
      _previous_instant_watt = data.instant_watt;
      update_required = true;
    }
  }

  // InstantAmpere
  if (data.instant_ampere) {
    if (!_previous_instant_ampere ||
        (_previous_instant_ampere->second.ampereR !=
         data.instant_ampere->second.ampereR) ||
        (_previous_instant_ampere->second.ampereT !=
         data.instant_ampere->second.ampereT)) {
      _previous_instant_ampere = data.instant_ampere;
      update_required = true;
    }
  }

  // CumlativeWattHour
  if (data.cumlative_watt_hour) {
    if (!_previous_cumlative_watt_hour ||
        (std::get<0>(*_previous_cumlative_watt_hour) !=
         std::get<0>(*data.cumlative_watt_hour))) {
      _previous_cumlative_watt_hour = data.cumlative_watt_hour;
      update_required = true;
    }
  }
  // 送信
  if (!update_required) {
    return;
  }
  JsonDocument doc;
  doc["instant_watt"] = _previous_instant_watt
                            ? _previous_instant_watt->second.watt.count()
                            : 0;
  doc["instant_ampere_R"] =
      _previous_instant_ampere
          ? std::chrono::duration_cast<ElectricityMeter::Ampere>(
                _previous_instant_ampere->second.ampereR)
                .count()
          : 0;
  doc["instant_ampere_T"] =
      _previous_instant_ampere
          ? std::chrono::duration_cast<ElectricityMeter::Ampere>(
                _previous_instant_ampere->second.ampereT)
                .count()
          : 0;
  auto &[p_cwh, p_coeff, p_unit] = *_previous_cumlative_watt_hour;
  auto p_kwh = EchonetLite::cumlative_kilo_watt_hour(p_cwh, p_coeff, p_unit);
  doc["cumlative_kwh"] = p_kwh.count();
  doc["measured_at"] = iso8601formatUTC(data.instant_watt->first);
  send_json(_endpoint, doc);
}
