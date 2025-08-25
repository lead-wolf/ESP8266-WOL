#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

// Cấu trúc dữ liệu thiết bị
struct Device {
  String name;
  String mac;
  String ip;
  String broadcast;
  int port;
};

const int NUM_DEVICES = 2;
Device devices[NUM_DEVICES] = {
  {"PC-AN", "1c:1b:0d:a9:ee:78", "10.10.10.80", "10.10.10.255", 4000},
  {"AN-SERVER", "70:5a:0f:b7:45:1e", "10.10.10.252", "10.10.10.255", 9},
};

// Thông tin mạng WiFi cố định
const char* ssid = "I'm Dev";
const char* password = "Password";

// Khởi tạo Web Server
ESP8266WebServer server(21001);
WiFiUDP udp;
unsigned long lastResetTime = 0;

bool isValidMAC(String mac) {
    int count = 0;
    for (int i = 0; i < mac.length(); i++) {
        if (isxdigit(mac[i])) { // Kiểm tra ký tự hex
            count++;
        } else if (mac[i] == ':') {
            if (count != 2 && count != 0) return false;
             count = 0;
        } else {
           return false;
        }
    }
    return count == 2;
}

// Hàm gửi gói tin WOL (sử dụng địa chỉ broadcast)
void sendWOL(String mac, String broadcast, int port) {
  if(!isValidMAC(mac)) {
    Serial.println("ERROR: Invalid MAC address format!");
    return; 
  }

  byte macBytes[6];
  macBytes[0] = strtol(mac.substring(0, 2).c_str(), NULL, 16);
  macBytes[1] = strtol(mac.substring(3, 5).c_str(), NULL, 16);
  macBytes[2] = strtol(mac.substring(6, 8).c_str(), NULL, 16); 
  macBytes[3] = strtol(mac.substring(9, 11).c_str(), NULL, 16);
  macBytes[4] = strtol(mac.substring(12, 14).c_str(), NULL, 16);
  macBytes[5] = strtol(mac.substring(15, 17).c_str(), NULL, 16);
  
  byte magicPacket[102];
  for (int i = 0; i < 6; i++) {
    magicPacket[i] = 0xFF;
  }
    
  for(int i = 6; i < 102; i += 6){
    for(int j = 0; j < 6; j++){
        magicPacket[i + j] = macBytes[j];
    }
  }
      
    IPAddress broadcastAddr;
    if(broadcast.equals("255.255.255.255")){
        broadcastAddr =  IPAddress(255,255,255,255);
    } else {
        broadcastAddr.fromString(broadcast);
    }

  udp.beginPacket(broadcastAddr, port);
  udp.write(magicPacket, 102);
  udp.endPacket();
}

// Hàm Ping
//bool pingDevice(String ip) {
//  IPAddress ipAddr;
//  ipAddr.fromString(ip);
//  bool ret = Ping.ping(ipAddr, 3);
//  delay(3000);
//  return ret;
//}

// Hàm Xử lý POST request/sendWOL
void handleSendWOL() {
    String password = server.arg("password");  
    String correctPassword = "protected"; // pass protected

    if (password != correctPassword) {
        server.send(400, "text/plain", "Mật khẩu không đúng!");
        Serial.println("Incorrect password");
        return; 
    }

    Serial.println("Start handleSendWOL");

    String selectedDeviceName = server.arg("device");
    Serial.print("Selected Device Name: ");
    Serial.println(selectedDeviceName);
    
    String selectedBroadcast = server.arg("broadcast");
    Serial.print("Selected Broadcast: ");
    Serial.println(selectedBroadcast);

    String selectedPortStr = server.arg("port");
    Serial.print("Selected Port String: ");
    Serial.println(selectedPortStr);
    int selectedPort = selectedPortStr.toInt();
    Serial.print("Selected Port int: ");
    Serial.println(selectedPort);

    int deviceIndex = -1;
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (devices[i].name == selectedDeviceName) {
            deviceIndex = i;
            break;
        }
    }
    if (deviceIndex == -1) {
       server.send(400, "text/plain", "Không tìm thấy thiết bị");
       Serial.println("Device not found!");
       return;
    }

    Device selectedDevice = devices[deviceIndex];
    Serial.print("Selected Device MAC: ");
    Serial.println(selectedDevice.mac);
    Serial.print("Selected Device broadcast: ");
    Serial.println(selectedDevice.broadcast);

    Serial.println("Sending WOL...");
    sendWOL(selectedDevice.mac, selectedDevice.broadcast, selectedDevice.port);

    server.send(200, "text/plain", "Đã gửi gói tin WOL");

//    int countPingFail = 0;
//    while (countPingFail <= 4) {
//        delay(5000);
//        if(pingDevice(selectedDevice.ip)) {
//            Serial.println("ping done!");
//            return;
//        }
//        Serial.println("ping false...");
//        countPingFail++;
//    }

    Serial.println("End handleSendWOL");
}

// Xử lý GET request /
void handleRoot() {
    String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                  "<link rel=\"icon\" href=\"https://cdn-icons-png.freepik.com/512/6329/6329326.png\" type=\"image/png\">"
                  "<title>WOL Controller</title>"
                  "<style>"
                  "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; background-color: #f4f4f9; }"
                  ".device-container { display: flex; justify-content: center; flex-wrap: wrap; gap: 20px; }"
                  ".device { width: 150px; height: 150px; border: 2px solid #ccc; border-radius: 10px; display: flex; flex-direction: column; justify-content: center; align-items: center; cursor: pointer; background-color: #fff; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }"
                  ".device:hover { background-color: #e8e8e8; }"
                  ".device img { width: 80px; height: 80px; margin-bottom: 10px; }"
                  ".device-name { font-size: 18px; font-weight: bold; }"
                  ".modal { display: none; position: fixed; z-index: 10; left: 0; top: -50px; width: 100%; height: 110%; background-color: rgba(0,0,0,0.5); justify-content: center; align-items: center; }"
                  ".modal-content { background: white; padding: 20px; border-radius: 10px; text-align: center; width: 300px; }"
                  ".modal-content input { width: 90%; padding: 10px; margin: 10px 0; font-size: 16px; border: 1px solid #ccc; border-radius: 5px; }"
                  ".modal-content button { padding: 10px 20px; font-size: 16px; background-color: #007BFF; color: white; border: none; border-radius: 5px; cursor: pointer; }"
                  ".modal-content button:hover { background-color: #0056b3; }"
                  "</style>"
                  "</head><body style=\""
                  "background-image: url(https://i.etsystatic.com/34157568/r/il/16822b/3670162370/il_fullxfull.3670162370_mz6y.jpg);"
                  "background-repeat: no-repeat;"
                  "background-size: cover;\">"
                  "<h1 style=\"font-size: 48px; margin-bottom: 50px;\">WOL Controller</h1>"
                  "<div class=\"device-container\">";

    // Tạo icon cho từng thiết bị
    for (int i = 0; i < NUM_DEVICES; i++) {
        const char* imageUrl;

        // Kiểm tra nếu tên chứa "PC"
        if (strstr(devices[i].name.c_str(), "PC") != NULL) {
            imageUrl = "https://cdn-icons-png.flaticon.com/512/9711/9711115.png"; 
        }
        // Kiểm tra nếu tên chứa "SERVER"
        else if (strstr(devices[i].name.c_str(), "SERVER") != NULL) {
            imageUrl = "https://cdn-icons-png.flaticon.com/512/1508/1508901.png"; 
        } else {
            imageUrl = "https://assets.streamlinehq.com/image/private/w_300,h_300,ar_1/f_auto/v1/icons/c5/no-image-px759fvb06aw65eaaqpopg.png/no-image-ousebmandtg1ym1w6nntl.png"; 
        }
      
        html += "<div class=\"device\" onclick=\"openModal('" + devices[i].name + "', '" + devices[i].broadcast + "', '" + devices[i].port + "')\">"
                "<img src=\"" + imageUrl + "\" alt=\"Device Icon\">" 
                "<div class=\"device-name\">" + devices[i].name + "</div>"
                "</div>";
    }

    html += "</div>"

            // Popup modal
            "<div id=\"modal\" class=\"modal\">"
            "<div class=\"modal-content\">"
            "<h2>Nhập Mật Khẩu</h2>"
            "<input type=\"password\" id=\"passwordInput\" placeholder=\"Mật khẩu\">"
            "<button onclick=\"sendWOL()\">Gửi</button>"
            "<button onclick=\"closeModal()\" style=\"background-color: #ccc; margin-left: 10px;\">Hủy</button>"
            "</div>"
            "</div>"

            "<script>"
            "let selectedDevice = {};"

            "function openModal(device, broadcast, port) {"
            "  selectedDevice = { device, broadcast, port };"
            "  document.getElementById('modal').style.display = 'flex';"
            "}"

            "function closeModal() {"
            "  document.getElementById('modal').style.display = 'none';"
            "  document.getElementById('passwordInput').value = '';"
            "}"

            "function sendWOL() {"
            "  const password = document.getElementById('passwordInput').value;"
            "  if (!password) { alert('Vui lòng nhập mật khẩu!'); return; }"
            "  const formData = new FormData();"
            "  formData.append('device', selectedDevice.device);"
            "  formData.append('broadcast', selectedDevice.broadcast);"
            "  formData.append('port', selectedDevice.port);"
            "  formData.append('password', password);"
            "  fetch('/sendWOL', { method: 'POST', body: formData })"
            "    .then(response => response.text())"
            "    .then(data => {"
            "      alert(data);"
            "      closeModal();"
            "    });"
            "}"
            // Lắng nghe sự kiện bàn phím khi modal mở
            "document.addEventListener('keydown', function (event) {"
            "  const modal = document.getElementById('modal');"
            "  if (modal.style.display === 'flex') {"
            "    if (event.key === 'Enter') {"
            "      sendWOL();"
            "    } else if (event.key === 'Escape') {"
            "      closeModal();"
            "    }"
            "  }"
            "});"
            "</script>"
            "</body></html>";

    server.send(200, "text/html", html);
}



void setup() {
  Serial.begin(115200);
  WiFi.hostname("ESP8266_WOL");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/sendWOL", HTTP_POST, handleSendWOL);
  server.begin();
  Serial.println("Web server started");

  lastResetTime = millis();
}

void loop() {
  server.handleClient();

  // Kiểm tra và khởi động lại sau mỗi tiếng
  if (millis() - lastResetTime >= 3600000) {
    Serial.println("Restarting device...");
    ESP.restart();
  }
}
