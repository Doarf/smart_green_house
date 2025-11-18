# smart_green_house

This project aims to design and build a smart mini-greenhouse using an embedded microcontroller system (ESP32).  The system autonomously monitors and controls environmental conditions such as soil moisture, air temperature, and, optionally, light intensity, to maintain optimal growing conditions for the plants.

Using environmental sensors, the system collects data and makes decisions based on configurable thresholds. It can automatically activate irrigation (pump) and, optionally, lighting (LEDs) to regulate the internal environment

| Component                      | Function                                                               |
| ------------------------------ | ---------------------------------------------------------------------- |
| **ESP32**                      | Main microcontroller (Wi-Fi, GPIO, ADC, UART, I2C)                     |
| **DHT22**                      | Measures ambient temperature and humidity                              |
| **DS18B20**                    | Measures soil temperature (waterproof digital sensor)                  |
| **OLED I2C Display (SSD1306)** | Real-time display of sensor readings and system status                 |
| **3 Push Buttons**             | Manual control (mode selection, threshold adjustment, manual watering) |
| **12V Water Pump**             | Automated irrigation system                                            |
| **Relay Module**               | Controls the 12V pump securely from the ESP32                          |
| **Power Supply (5V/12V)**      | Powers ESP32 and pump                                                  |
