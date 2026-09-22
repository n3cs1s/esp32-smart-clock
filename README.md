# ESP32 Smart Clock

An ESP32-WROOM-32 smart desk clock with a 16×2 LCD, local Wi-Fi setup portal, web controls, countdown timer, daily alarm, buzzer notifications, and a MEMS microphone input.

The project is designed as an approachable Arduino IDE project: no cloud account, mobile app, or external server is required for the clock, timer, and alarm features.

## Features

- **Accurate Kyiv local time** synchronized over Wi-Fi with NTP, including daylight-saving time changes.
- **16×2 parallel LCD interface** showing the current time and timer or alarm status.
- **Wi-Fi setup access point** for first-time configuration:
  - ESP32 creates its own setup network.
  - A phone or computer can open a local configuration page.
  - Available home Wi-Fi networks are scanned and listed.
  - The selected network name and password are saved in ESP32 non-volatile storage.
- **Local web interface** available from a browser on the same network.
- **Countdown timer** with create, start, stop, and clear actions.
- **Daily alarm clock** with configurable hour and minute, plus enable, disable, and clear actions.
- **Buzzer notification** when a timer completes or an alarm triggers.
- **Analog MEMS microphone input** on an ADC pin, with an included sound-level measurement routine for future sound-reactive features.

## Hardware

| Part | Purpose |
| --- | --- |
| ESP32-WROOM-32 development board | Main controller and Wi-Fi server |
| 16×2 HD44780-compatible LCD | Time, date, timer, and alarm display |
| 10 kΩ potentiometer | LCD contrast adjustment |
| Active buzzer | Timer and alarm notification |
| Analog MEMS microphone module | Sound-level input |
| 0.1 µF capacitor | Recommended decoupling capacitor for the microphone module |

## Wiring

### LCD in 4-bit parallel mode

| LCD pin | Signal | ESP32 connection |
| --- | --- | --- |
| 1 | VSS | GND |
| 2 | VDD | 3.3 V for the basic test setup |
| 3 | VO | Centre pin of the 10 kΩ contrast potentiometer |
| 4 | RS | GPIO 23 |
| 5 | RW | GND |
| 6 | E | GPIO 22 |
| 11 | D4 | GPIO 21 |
| 12 | D5 | GPIO 19 |
| 13 | D6 | GPIO 18 |
| 14 | D7 | GPIO 17 |
| 15 | A / LED+ | 5 V through a 100–220 Ω series resistor |
| 16 | K / LED− | GND |

Connect the outer potentiometer terminals to **GND** and the LCD supply voltage; connect its centre terminal to LCD **VO** (pin 3).

The LCD backlight is wired as **5 V → 100–220 Ω resistor → LCD A / LED+ (pin 15)**. Connect LCD **K / LED− (pin 16)** directly to GND. The resistor limits the backlight current and protects the LED.

> If the LCD logic is powered from 5 V, a 3.3 V to 5 V level shifter is recommended for RS, E, and D4–D7. Do not feed 5 V into ESP32 GPIO pins.

### Other modules

| Module pin | ESP32 connection |
| --- | --- |
| MEMS microphone VCC | 3.3 V only |
| MEMS microphone GND | GND |
| MEMS microphone AUD | GPIO 34 |
| Active buzzer signal | GPIO 26 |
| Active buzzer GND | GND |

Place a 0.1 µF ceramic capacitor between the microphone module's **VCC** and **GND**, close to the module, to reduce supply noise.

## Getting Started

1. Install the ESP32 board package in Arduino IDE.
2. Select **ESP32 Dev Module** and the correct serial port.
3. Install or use the built-in libraries required by the sketch:
   - `WiFi.h`
   - `WebServer.h`
   - `Preferences.h`
   - `LiquidCrystal.h`
   - `time.h`
4. Upload the sketch.
5. Open Serial Monitor at **115200 baud**.
6. Connect a phone or computer to the ESP32 setup Wi-Fi network.
7. Open `http://192.168.4.1` in a browser.
8. Choose the home Wi-Fi network, enter its password, and save.
9. Find the device's home-network IP address in Serial Monitor and open it in a browser.

## Web Controls

The local setup page provides:

- Home Wi-Fi network selection and password entry
- Wi-Fi credential reset
- Timer duration configuration in minutes and seconds
- Timer start, stop, and clear controls
- Alarm time configuration in hours and minutes
- Alarm enable, disable, and clear controls

## LCD Behaviour

The first row shows the current local time.

The second row shows, in order of priority:

1. Countdown timer status, when a timer exists
2. Alarm configuration, when no timer exists but an alarm exists
3. Current date, when neither exists

## Current Limitations

- Timer and alarm settings are stored in RAM and reset after ESP32 restarts.
- The Wi-Fi access point remains available after the ESP32 joins the home network; change the default setup AP password before regular use.
- The microphone code currently measures sound level only. It does not perform speech-to-text or voice-command recognition.
- The buzzer logic is intended for an active buzzer. A passive buzzer needs a tone-generation routine for audible melodies.

## Ideas for Future Development

- Save timer and alarm settings using `Preferences`
- Add a button to silence an active alarm
- Add a web endpoint with live JSON clock and sensor data
- Display sound level on the web page
- Add temperature, humidity, or light sensors
- Add an RTC module such as DS3231 for timekeeping without Wi-Fi
- Add voice-command recognition for hands-free timer and alarm control; an ESP32-S3 with PSRAM and an I²S microphone is recommended for offline command recognition

## License

Choose a license before publishing. The [MIT License](https://choosealicense.com/licenses/mit/) is a simple option for an open hardware/software learning project.
