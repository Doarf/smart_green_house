# smart_green_house
This project aims to design and build a smart mini-greenhouse using an ESP32 embedded microcontroller. The system autonomously monitors and controls environmental conditions such as soil moisture, air temperature, and optionally light intensity, in order to maintain optimal plant growth conditions.

Using environmental sensors, the system collects data and makes decisions based on configurable thresholds. It can automatically control irrigation (water pump) and optionally lighting (LED) to regulate the greenhouse environment.

| **Component**                   | **Function**                                 |
| ------------------------------- | -------------------------------------------- |
| **ESP32**                       | Main microcontroller (Wi-Fi, GPIO, ADC, I2C) |
| **DHT22**                       | Air temperature and humidity sensing         |
| **DS18B20**                     | Soil temperature sensing                     |
| **OLED Display (SSD1306, I2C)** | Displays environmental data                  |
| **Push Buttons**                | User interface and system control            |
| **12V Water Pump**              | Automatic plant irrigation                   |
| **Relay Module**                | Controls the water pump                      |
| **5V Power Supply**             | Powers the ESP32 and peripherals             |
