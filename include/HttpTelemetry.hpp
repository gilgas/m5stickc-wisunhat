// Copyright (c) 2022 Akihiro Yamamoto.
// Licensed under the MIT License <https://spdx.org/licenses/MIT.html>
// See LICENSE file in the project root for full license information.
//
#pragma once
#include "Repository.hpp"
#include <optional>
#include <string>

//
//
//
class HttpTelemetry final {
public:
  HttpTelemetry(std::string endpoint);
  void task_handler();

private:
  const std::string _endpoint;
  std::optional<Repository::InstantWatt> _previous_instant_watt;
  std::optional<Repository::InstantAmpere> _previous_instant_ampere;
  std::optional<Repository::CumlativeWattHour> _previous_cumlative_watt_hour;
};