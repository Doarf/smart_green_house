# smart_green_house

Ce projet vise à concevoir et construire une mini-serre intelligente à l'aide d'un système de microcontrôleur embarqué (ESP32). Ce système surveille et contrôle de manière autonome les conditions environnementales telles que l'humidité du sol, la température de l'air et, en option, l'intensité lumineuse, afin de maintenir des conditions de croissance optimales pour les plantes.

Grâce à des capteurs environnementaux, le système collecte des données et prend des décisions en fonction de seuils configurables. Il peut activer automatiquement l'irrigation (pompe) et, en option, l'éclairage (LED) pour réguler l'environnement intérieur.

| Component                      | Function                                                               |
| ------------------------------ | ---------------------------------------------------------------------- |
| **ESP32**                      | Microcontrôleur (Wi-Fi, GPIO, ADC, UART, I2C)                          |
| **DHT22**                      | Capteur de température et d'humidité (ambiant)                         |
| **DS18B20**                    | Capteur de température (terre)                                         |
| **OLED I2C Display (SSD1306)** | Ecran oled en I2C                                                      |
| **3 Boutons     **             | Bouton pour controler la serre                                         |
| **12V Pompe à eau**            | Automatisé l'eau pour la terre                                         |
| **Relay Module**               | Pour controler la pompe à eau                                          |
| **Power Supply (5V)**          | Alimenter l'ESP32 et la pompe à eau                                    |
