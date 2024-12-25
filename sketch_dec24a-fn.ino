#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

// Cấu trúc dữ liệu thiết bị (thêm trường broadcast)
struct Device {
  String name;
  String mac;
  String ip;
  String broadcast;
  int port;
  int ssidIndex;
};

// Cấu trúc dữ liệu mạng WiFi
struct SSID {
  String ssid;
  String password;
};

const int NUM_DEVICES = 2;
Device devices[NUM_DEVICES] = {
  {"PC-1", "01:23:45:67:89:AB", "10.10.10.10", "10.10.10.255", 4000, 0},
  {"PC-2", "CD:EF:01:23:45:67", "192.168.1.11", "192.168.1.255", 9, 1},
};

// Thông tin các mạng WiFi, thay đổi cho phù hợp
const int NUM_SSIDS = 2;
SSID ssids[NUM_SSIDS] = {
  {"ssid1", "pass1"},
  {"ssid2", "pass2"},
};

int currentSSIDIndex = 0;
String currentSSID;

// Khởi tạo Web Server
ESP8266WebServer server(21001);
WiFiUDP udp;

bool isValidMAC(String mac) {
    int count = 0;
    for (int i = 0; i < mac.length(); i++) {
        if (isxdigit(mac[i])) { // Kiểm tra ký tự hex
            count++;
        } else if (mac[i] == ':') {
            if (count != 2 && count != 0) return false;
             count = 0;
        }else{
           return false;
        }
    }

  if(count !=2){
     return false;
  }
    return true;
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
     }else{
          broadcastAddr.fromString(broadcast);
    }

  udp.beginPacket(broadcastAddr, port);
  udp.write(magicPacket, 102);
  udp.endPacket();
}


// Hàm Ping
bool pingDevice(String ip){
  IPAddress ipAddr;
  ipAddr.fromString(ip);
  bool ret = Ping.ping(ipAddr, 3);
  delay(3000);
  return ret;
}

// Hàm Xử lý POST request/sendWOL
void handleSendWOL() {

    String password = server.arg("password");  
    String correctPassword = ""; //pass protected

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

    int selectedSSIDIndex = server.arg("ssidIndex").toInt();
    Serial.print("Selected SSID Index: ");
    Serial.println(selectedSSIDIndex);


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

    Serial.print("Current SSID index: ");
    Serial.println(currentSSIDIndex);

    bool changeSSID = false;
    if(selectedDevice.ssidIndex != currentSSIDIndex){
        currentSSIDIndex = selectedDevice.ssidIndex;
        WiFi.begin(ssids[currentSSIDIndex].ssid, ssids[currentSSIDIndex].password);
        Serial.println("Connecting to new SSID");

        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
          delay(500);
          Serial.print(".");
           timeout++;
           if (timeout > 20) {
              Serial.println("Can't connect to new wifi!");
              bool changeSSID = false;
              return;
          }
         }
        Serial.println("\nConnected to SSID => done!");
        bool changeSSID = true;
    }
    
    Serial.println("Sending WOL...");
    sendWOL(selectedDevice.mac, selectedBroadcast, selectedPort);

    if(changeSSID){
      WiFi.begin(ssids[0].ssid, ssids[0].password);
        Serial.println("Connecting to default SSID");


        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
          delay(500);
          Serial.print(".");
           timeout++;
           if (timeout > 20) {
              Serial.println("Can't connect to new wifi!");
              return;
          }
         }
        Serial.println("\nConnected to SSID => done!");
    }

    server.send(200, "text/plain", "Đã gửi gói tin WOL");

    int countPingFail = 0;
    while (countPingFail <= 4) {
          delay(5000);
         if(pingDevice(selectedDevice.ip)){
              Serial.println("ping done!");
              return;
         }
          Serial.println("ping false...");
          countPingFail++;
     }

  Serial.println("End handleSendWOL");
}

// Xử lý GET request /
void handleRoot() {
    String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                  "<title>WOL Controller</title></head><body style=\"font-size: 42px;\"><h1 style=\"justify-content: center; display: flex; margin: 40px;\">WOL Controller</h1>"
                  "<form method=\"post\" action=\"/sendWOL\" style=\"justify-self: center\">"
                  "<label for=\"deviceSelect\">Chọn thiết bị:</label>"
                  "<select id=\"deviceSelect\" name=\"device\" onchange=\"updateForm(this)\" style=\"margin-bottom: 40px; margin-left: 120px; font-size: 24px;\">"; 
                  
      for (int i = 0; i < NUM_DEVICES; i++) {
          html += "<option value=\"" + devices[i].name + "\" data-broadcast=\"" + devices[i].broadcast + "\" data-port=\"" + devices[i].port + "\">" + devices[i].name + "</option>";
      }

     html +=  "</select><br><label for=\"broadcastInput\">Địa chỉ Broadcast:</label>"
                "<input type=\"text\" id=\"broadcastInput\" name=\"broadcast\" value=\"" + devices[0].broadcast + "\" style=\"margin-bottom: 40px; margin-left: 40px; font-size: 24px;\"><br>"
                 "<label for=\"portInput\" >Cổng:</label>"
                "<input type=\"number\" id=\"portInput\" name=\"port\" value=\"" + String(devices[0].port) + "\" style=\"margin-bottom: 40px; margin-left: 250px; font-size: 24px;\"><br>"
                "<label for=\"wifiSelect\">Mạng WiFi:</label>"
                 "<select id=\"wifiSelect\" name=\"ssidIndex\" style=\"margin-bottom: 40px; margin-left: 150px; font-size: 24px;\">";

     for(int i=0; i < NUM_SSIDS; i++){
         html += "<option value=\""+ String(i) + "\" >" + ssids[i].ssid + "</option>";
     }

         html+= "</select><br><label for=\"passwordInput\">Mật khẩu:</label>"
                "<input type=\"password\" id=\"passwordInput\" name=\"password\" style=\"margin-bottom: 40px; margin-left: 40px; font-size: 24px;\"><br>" ;

    html += "</select><br><button type=\"submit\" style=\"font-size: 24px;margin-left: 600px\">Gửi WOL</button></form>"
                "<div id=\"statusMessage\"></div>"
                "<script>"
                "const statusMessage = document.getElementById('statusMessage');"
                "const form = document.querySelector('form');"
                 "const broadcastInput = document.getElementById('broadcastInput');"
                 "const portInput = document.getElementById('portInput');"

                "function updateForm(select) {"
                  "const selectedOption = select.options[select.selectedIndex];"
                  "const broadcastValue = selectedOption.getAttribute('data-broadcast');"
                  "const portValue = selectedOption.getAttribute('data-port');"
                  "broadcastInput.value = broadcastValue;" 
                   "portInput.value = portValue;"

                "}"
                "form.addEventListener('submit', async function(event) {"
                "event.preventDefault(); statusMessage.textContent = 'Đang xử lý...';"
                "const formData = new FormData(form);"
                "const response = await fetch('/sendWOL', {method: 'POST', body: formData});"
                "const data = await response.text();statusMessage.textContent = data;});"
                "</script></body></html>";

    server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
    WiFi.hostname("ESP8266_WOL");
    WiFi.begin(ssids[0].ssid, ssids[0].password);
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED) {
         delay(500);
        Serial.print(".");
          timeout++;
           if (timeout > 20) {
              Serial.println("Không thể kết nối wifi!");
               return;
           }
    }
    currentSSID = ssids[0].ssid;
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/sendWOL", HTTP_POST, handleSendWOL);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}