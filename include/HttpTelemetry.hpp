// Copyright (c) 2022 Akihiro Yamamoto.
// Licensed under the MIT License <https://spdx.org/licenses/MIT.html>
// See LICENSE file in the project root for full license information.
//
#pragma once
#include "Repository.hpp"
#include <optional>

//
//
//
class HttpTelemetry final {
public:
  HttpTelemetry();
  void task_handler();

private:
  std::optional<Repository::InstantWatt> _previous_instant_watt;
};