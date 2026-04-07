#pragma once

#include <Arduino.h>

namespace AppConfig
{

  constexpr char AP_SSID[] = "ESP32-Assarte";
  constexpr char AP_PASSWORD[] = "assarte123";

  // Fill these in if you want the device to also join an existing Wi-Fi network.
  constexpr char STA_SSID[] = "";
  constexpr char STA_PASSWORD[] = "";

  constexpr uint8_t DOOR_PIN = 26;
  constexpr bool DOOR_ACTIVE_HIGH = true;
  constexpr bool DOOR_DEFAULT_OPEN = false;

  constexpr uint8_t LIGHT_PIN = 27;
  constexpr bool LIGHT_ACTIVE_HIGH = true;
  constexpr bool LIGHT_DEFAULT_ON = false;

  constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

} // namespace AppConfig
