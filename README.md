# ESP32 Assarte

## Configure

Edit [`include/app_config.h`](/home/igor/Developments/esp32-assarte/include/app_config.h):

- Wi-Fi credentials if you want the ESP32 on your network
- AP credentials if you want to change them
- GPIO pins for door and light

## Run

Install frontend dependencies once:

```sh
pnpm --dir interface install
```

Upload everything:

```sh
pio run -t upload
```

That uploads:

- firmware
- the static frontend
- LittleFS files

If you use the PlatformIO VSCode extension, the normal `Upload` button does the same job.

## Serial Monitor

```sh
pio device monitor
```

Baud rate is `115200`.

## Open The UI

If you use the ESP32 access point:

- SSID: `ESP32-Assarte`
- Password: `assarte123`
- URL: `http://192.168.4.1`

If you configured your Wi-Fi in `app_config.h`, open the serial monitor and look for:

```text
Station connected: <ip>
```

Then open that IP in your browser.

## Useful Commands

Build firmware only:

```sh
pio run
```

Build filesystem image only:

```sh
pio run -t buildfs
```

Upload filesystem only:

```sh
pio run -t uploadfs
```
