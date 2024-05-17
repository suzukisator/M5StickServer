#include <M5StickCPlus2.h>
#include <Kalman.h>
#include <WiFi.h>

//大学wifiルーター
/*
const char* ssid = "ASUS_28_2G";
const char* password = "morning_6973";
const char* server_ip = "192.168.50.24";
*/
const char* ssid = "Buffalo-G-1AF0";
const char* password = "7nyh4sj46px64";
const char* server_ip = "192.168.11.4";

const int server_port = 3002;
const int device_id = 1;
const int interval = 10000;

unsigned long prevTime = 0, startTime = 0;
float filteredacc[3] = {0, 0, 0};
float sumAcc[3] = {0, 0, 0};
float meanacc[3] = {0, 0, 0};
float sumnormacc = 0;
float meannormacc = 0;
float normacc = 0;
int count = 0;

Kalman kalmanX;
Kalman kalmanY;
Kalman kalmanZ;

WiFiClient wifiClient;

//ディスプレイ表示
void displayConnectionStatus(const char* status) {
    StickCP2.Display.println(status);
}

//wifi接続
void setupWiFi() {
    StickCP2.Display.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    StickCP2.Display.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
    displayConnectionStatus("Failed to connect WiFi");
    while(1) delay(1000);
    } else {
    displayConnectionStatus("WiFi connected");
    }
}

//サーバーとの接続確認
bool connectToServer() {
  if (!wifiClient.connect(server_ip, server_port)) {
    displayConnectionStatus("server conn failed");
    return false;
  } else {
    displayConnectionStatus("Connected to server");
    return true;
  }
}

//送信データ作成
void sendData() {
  if (wifiClient.connected()) {
    byte data[24];

    //int deviceIdNetworkOrder = htonl(device_id);
    float m5time = millis() / 1000.0f;
    *((int*)data) = device_id;
    *((float*)(data + 4)) = filteredacc[0];
    *((float*)(data + 8)) = filteredacc[1];
    *((float*)(data + 12)) = filteredacc[2];
    *((float*)(data + 16)) = normacc;
    *((float*)(data + 20)) = m5time;

    wifiClient.write(data, sizeof(data));
  } else {
    wifiClient.stop();
    while (!connectToServer()) {
      displayConnectionStatus("Reconnection failed. Trying again...");
      delay(1000);
    }
    displayConnectionStatus("Reconnected to server");
  }
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Imu.init();
    StickCP2.Display.setBrightness(128); // 画面の明るさを半分に設定
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextSize(2);
    StickCP2.Display.printf("Device_ID : %d\n", device_id);
    StickCP2.Display.printf("Battery : %d % \n", StickCP2.Power.getBatteryLevel());
    setupWiFi();
    if (connectToServer()) {
        displayConnectionStatus("Connected to server");
    }

    prevTime = millis();
    startTime = millis();
}

void loop(void) {
    StickCP2.Imu.update();
    unsigned long currentTime = millis();
    float dt = (currentTime - prevTime) / 1000.0f;
    prevTime = currentTime;

    if (dt > 0) {
        auto data = StickCP2.Imu.getImuData();
        filteredacc[0] = data.accel.x;
        filteredacc[1] = data.accel.y;
        filteredacc[2] = data.accel.z;

        normacc = sqrt(filteredacc[0]*filteredacc[0] + filteredacc[1]*filteredacc[1] + filteredacc[2]*filteredacc[2]);
        if (interval <= currentTime - startTime) {
          StickCP2.Display.fillScreen(BLACK);
          StickCP2.Display.setCursor(0, 0);
          StickCP2.Display.printf("Device_ID : %d", device_id);
          StickCP2.Display.printf("Battery : %d %", StickCP2.Power.getBatteryLevel());
          startTime = currentTime;
        }
        sendData();
    }
    delay(50);
}
