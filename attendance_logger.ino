#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

// ========== Wi-Fi Access Point credentials ==========
const char* ssid = "Attendance Point";
const char* password = "joinme";

// ========== Student Mapping ==========
struct Student {
  const char* mac;
  const char* name;
  const char* roll;
};

Student studentList[] = {
  { "44:EA:30:7F:B0:0D", "red watch", "2023UED101" },
  { "DE:AD:BE:EF:00:01", "Bob", "2023UED102" },
  { "12:34:56:78:9A:BC", "Charlie", "2023UED103" },
  { "EE:B4:CF:79:4C:19", "Namai", "23f3000200" },
  { "CA:04:A9:9B:0E:F2", "Prof", "1" }
};
const int studentCount = sizeof(studentList) / sizeof(Student);

// ========== Attendance list ==========
#define MAX_CLIENTS 100
struct MacEntry {
  String mac;
  unsigned long timestamp;
};
MacEntry seenMACs[MAX_CLIENTS];
int macCount = 0;

// ========== Web server ==========
WebServer server(80);

// ========== HTML Page ==========
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset='UTF-8'>
      <title>Attendance Logger</title>
      <style>
        body {
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
          background: #f4f6f8;
          color: #333;
          padding: 20px;
          text-align: center;
        }
        h1 {
          color: #2c3e50;
          margin-bottom: 20px;
        }
        ol {
          text-align: left;
          display: inline-block;
          background: #fff;
          padding: 30px 40px;
          width: 90%;
          max-width: 600px;
          border-radius: 12px;
          box-shadow: 0 0 12px rgba(0,0,0,0.1);
          margin-bottom: 30px;
        }
        li {
          padding: 10px 0;
          font-size: 16px;
        }
        input[type="text"] {
          padding: 10px;
          width: 80%;
          max-width: 400px;
          border: 1px solid #ccc;
          border-radius: 8px;
          margin: 10px 0;
          font-size: 16px;
        }
        button {
          margin-top: 20px;
          padding: 12px 30px;
          background-color: #3498db;
          color: white;
          border: none;
          border-radius: 50px;
          font-size: 16px;
          cursor: pointer;
          transition: background-color 0.3s ease;
        }
        button:hover {
          background-color: #2980b9;
        }
      </style>
    </head>
    <body>
      <h1>Attendance Logger</h1>
      <ol>
  )rawliteral";

  for (int i = 0; i < macCount; i++) {
    String name = "Unknown";
    String roll = "";
    for (int j = 0; j < studentCount; j++) {
      if (seenMACs[i].mac.equalsIgnoreCase(studentList[j].mac)) {
        name = studentList[j].name;
        roll = studentList[j].roll;
        break;
      }
    }
    html += "<li>" + name + " - " + roll + " (" + seenMACs[i].mac + ")</li>";
  }

  html += R"rawliteral(
      </ol>
      <h3>Enter Subject and Date</h3>
      <input type="text" id="subject" placeholder="Subject" /><br>
      <input type="text" id="date" placeholder="Date (e.g. 2025-05-15)" /><br>
      <button onclick="downloadAttendance()">Download Attendance</button>

      <script>
        function downloadAttendance() {
          let subject = document.getElementById('subject').value.trim();
          let date = document.getElementById('date').value.trim();
          if (!subject || !date) {
            alert("Please enter both subject and date.");
            return;
          }
          let url = `/download?subject=${encodeURIComponent(subject)}&date=${encodeURIComponent(date)}`;
          fetch(url)
            .then(resp => resp.blob())
            .then(blob => {
              let link = document.createElement('a');
              link.href = URL.createObjectURL(blob);
              link.download = "attendance.txt";
              link.click();
            });
        }
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// ========== Attendance Download ==========
void handleDownload() {
  String subject = server.hasArg("subject") ? server.arg("subject") : "Unknown Subject";
  String date = server.hasArg("date") ? server.arg("date") : "Unknown Date";

  String content = "Attendance: " + subject + " " + date + "\n\n";

  for (int i = 0; i < macCount; i++) {
    String name = "Unknown";
    String roll = "";
    for (int j = 0; j < studentCount; j++) {
      if (seenMACs[i].mac.equalsIgnoreCase(studentList[j].mac)) {
        name = studentList[j].name;
        roll = studentList[j].roll;
        break;
      }
    }
    content += name + "\t" + roll + "\t(" + seenMACs[i].mac + ")\n";
  }

  server.sendHeader("Content-Type", "text/plain");
  server.sendHeader("Content-Disposition", "attachment; filename=\"attendance.txt\"");
  server.send(200, "text/plain", content);
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started at: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.begin();
}

// ========== Loop: MAC Detection + Auto-Disconnect ==========
void loop() {
  server.handleClient();

  wifi_sta_list_t stationList;
  if (esp_wifi_ap_get_sta_list(&stationList) == ESP_OK) {
    for (int i = 0; i < stationList.num; i++) {
      wifi_sta_info_t station = stationList.sta[i];
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               station.mac[0], station.mac[1], station.mac[2],
               station.mac[3], station.mac[4], station.mac[5]);

      bool exists = false;
      for (int j = 0; j < macCount; j++) {
        if (seenMACs[j].mac.equalsIgnoreCase(macStr)) {
          exists = true;

          // If connected > 3 seconds, disconnect
          unsigned long currentMillis = millis();
          if (currentMillis - seenMACs[j].timestamp > 3000) {
            esp_wifi_deauth_sta(station.mac, WIFI_REASON_UNSPECIFIED);
            Serial.println("Disconnected: " + String(macStr));
          }

          break;
        }
      }

      if (!exists && macCount < MAX_CLIENTS) {
        seenMACs[macCount].mac = String(macStr);
        seenMACs[macCount].timestamp = millis();
        Serial.println("MAC Registered: " + String(macStr));
        macCount++;
      }
    }
  }

  delay(1000); // scan every 1 second
}
