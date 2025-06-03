# Project-MAL-
MAC Address based Attendance Logger 

📶 ESP32 Attendance Logger via Wi-Fi (MAC-based)

This is a lightweight and efficient ESP32-based attendance logger that uses MAC address detection over Wi-Fi. Devices connect to a local ESP32 access point, get identified by MAC, logged for attendance, and are then automatically disconnected after 3 seconds to reduce bandwidth/load.

✨ Features

✅ ESP32 acts as a Wi-Fi hotspot named Attendance Point
✅ Auto-detects connected devices' MAC addresses
✅ Cross-references known student MACs with names and roll numbers
✅ Displays live attendance list on a web interface
✅ Allows subject + date input and lets you download attendance as a .txt file
✅ Devices are automatically disconnected ~3 seconds after being logged to avoid congestion


🔧 Hardware Requirements
ESP32 development board
Power source (USB)
A mobile or device with Wi-Fi to connect and register
🧑‍💻 Setup Instructions
Clone this repo or copy attendance_logger.ino into your Arduino IDE

Edit the studentList[] array to include MAC addresses of known students
Upload the code to your ESP32
ESP32 starts an access point Attendance Point 
Students connect briefly (3 seconds), get logged, and auto-disconnected

Visit 192.168.4.1 in a browser to view/download attendance
