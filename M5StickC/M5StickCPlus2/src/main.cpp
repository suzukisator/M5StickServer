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
const int device_id = 2;
const int interval = 10000;

unsigned long prevTime = 0, startTime = 0;
float filteredacc[3] = {0, 0, 0};
float sumAcc[3] = {0, 0, 0};
float meanacc[3] = {0, 0, 0};
float sumnormacc = 0;
float meannormacc = 0;
int count = 0;

Kalman kalmanX;
Kalman kalmanY;
Kalman kalmanZ;

WiFiClient wifiClient;

//ディスプレイ表示
void displayConnectionStatus(const char* status) {
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.setCursor(0, 0);
    StickCP2.Display.println("m5stickc2 ver 0.42");
    StickCP2.Display.print("Device_ID : ");
    StickCP2.Display.println(device_id);
    StickCP2.Display.print("interval : ");
    StickCP2.Display.print(interval);
    StickCP2.Display.println("ms");
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
    *((float*)(data + 4)) = meanacc[0];
    *((float*)(data + 8)) = meanacc[1];
    *((float*)(data + 12)) = meanacc[2];
    *((float*)(data + 16)) = meannormacc;
    *((float*)(data + 20)) = m5time;
    /*
    memcpy(data, &deviceIdNetworkOrder, sizeof(deviceIdNetworkOrder));
    memcpy(data + 4, &meanacc[0], sizeof(meanacc[0]));
    memcpy(data + 8, &meanacc[1], sizeof(meanacc[1]));
    memcpy(data + 12, &meanacc[2], sizeof(meanacc[2]));
    memcpy(data + 16, &meannormacc, sizeof(meannormacc));
    memcpy(data + 20, &m5time, sizeof((m5time)));
    */

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
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Imu.init();
    StickCP2.Display.setTextSize(1);
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
        filteredacc[0] = kalmanX.getAngle( data.accel.x, data.gyro.x, dt);
        filteredacc[1] = kalmanY.getAngle(data.accel.y, data.gyro.y, dt);
        filteredacc[2] = kalmanZ.getAngle(data.accel.z, data.accel.z, dt);

        float normacc = sqrt(filteredacc[0]*filteredacc[0] + filteredacc[1]*filteredacc[1] + filteredacc[2]*filteredacc[2]);

        for (int i = 0; i < 3; i++) {
        sumAcc[i] += filteredacc[i];
        sumnormacc += normacc;
        }
        count++;

        if (currentTime - startTime >= interval) {
        for (int i = 0; i < 3; i++) {
        meanacc[i] = sumAcc[i] / count;
        }
        meannormacc = sumnormacc / count;
        sendData();
        for (int i = 0; i < 3; i++) {
            sumAcc[i] = 0;
        }
        sumnormacc = 0;
        count = 0;
        startTime = currentTime;
        }
    }
    delay(50);
    /*
    auto imu_update = StickCP2.Imu.update();
    if (imu_update) {
        StickCP2.Display.setCursor(0, 40);
        StickCP2.Display.clear();  // Delay 100ms 延迟100ms

        auto data = StickCP2.Imu.getImuData();

        // The data obtained by getImuData can be used as follows.
        // data.accel.x;      // accel x-axis value.
        // data.accel.y;      // accel y-axis value.
        // data.accel.z;      // accel z-axis value.
        // data.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.

        // data.gyro.x;      // gyro x-axis value.
        // data.gyro.y;      // gyro y-axis value.
        // data.gyro.z;      // gyro z-axis value.
        // data.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.

        // data.value;  // all sensor 9values array [0~2]=accel / [3~5]=gyro /
        //              // [6~8]=mag

        Serial.printf("ax:%f  ay:%f  az:%f\r\n", data.accel.x, data.accel.y,
                      data.accel.z);
        Serial.printf("gx:%f  gy:%f  gz:%f\r\n", data.gyro.x, data.gyro.y,
                      data.gyro.z);

        StickCP2.Display.printf("IMU:\r\n");
        StickCP2.Display.printf("%0.2f %0.2f %0.2f\r\n", data.accel.x,
                                data.accel.y, data.accel.z);
        StickCP2.Display.printf("%0.2f %0.2f %0.2f\r\n", data.gyro.x,
                                data.gyro.y, data.gyro.z);
    }
    delay(100);
    */
}
