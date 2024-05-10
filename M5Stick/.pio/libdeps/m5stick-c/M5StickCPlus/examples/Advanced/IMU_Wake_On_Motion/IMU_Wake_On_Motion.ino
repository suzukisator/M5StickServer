/*
*******************************************************************************
* Copyright (c) 2023 by M5Stack
*                  Equipped with M5StickCPlus sample source code
*                          配套  M5StickCPlus 示例源代码
* Visit for more information: https://docs.m5stack.com/en/core/m5stickc_plus
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/m5stickc_plus
*
* Describe:  Wake-On-Motion.  MPU6886运动唤醒
* Date: 2023/7/21
*******************************************************************************
The device automatically enters deep sleep mode after 5s of inactivity, and
prompts the cause of wakeup when it wakes up.
设备在静止状态下停留5s后,自动进入深度睡眠模式,并在唤醒时提示造成唤醒的原因.
*/
#include <M5StickCPlus.h>
#include <driver/rtc_io.h>  // from ESP-IDF

void mpu6886_wake_on_motion_isr(void);  // declaration of ISR

#define WOM_ATTACH_ISR
volatile uint32_t g_wom_count       = 0;
volatile uint32_t g_wom_last_millis = 0;
void IRAM_ATTR mpu6886_wake_on_motion_isr(void) {
    g_wom_count++;
    g_wom_last_millis = millis();
}

/* Method to print the reason by which ESP32 has been awoken from sleep */
void get_wakeup_reason_string(char *cbuf, int cbuf_len) {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            snprintf(cbuf, cbuf_len, "ext0");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            snprintf(cbuf, cbuf_len, "ext1");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            snprintf(cbuf, cbuf_len, "timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            snprintf(cbuf, cbuf_len, "touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            snprintf(cbuf, cbuf_len, "ULP");
            break;
        default:
            snprintf(cbuf, cbuf_len, "%d", wakeup_reason);
            break;
    }
}

RTC_DATA_ATTR int bootCount = 0;
#define WAKE_REASON_BUF_LEN 100

void setup() {
    char wake_reason_buf[WAKE_REASON_BUF_LEN];

    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);

    get_wakeup_reason_string(wake_reason_buf, WAKE_REASON_BUF_LEN);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("WOM:BOOT=%d,SRC=%s", bootCount, wake_reason_buf);
    Serial.printf("WOM:BOOT=%d,SRC=%s\n", bootCount, wake_reason_buf);
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.printf("Battery: ");
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("V: %.3f v", M5.Axp.GetBatVoltage());
    M5.Lcd.setCursor(0, 60);
    M5.Lcd.printf("I: %.3f ma", M5.Axp.GetBatCurrent());
    M5.Lcd.setCursor(0, 80);
    M5.Lcd.printf("P: %.3f mw", M5.Axp.GetBatPower());

#ifdef WOM_GPIO_DEBUG_TEST
    pinMode(GPIO_NUM_26, OUTPUT);
    pinMode(GPIO_NUM_36, INPUT);
#endif  // #ifdef WOM_GPIO_DEBUG_TEST

#ifdef WOM_ATTACH_ISR
    // set up ISR to trigger on GPIO35
    delay(100);
    pinMode(GPIO_NUM_35, INPUT);
    delay(100);
    attachInterrupt(GPIO_NUM_35, mpu6886_wake_on_motion_isr, FALLING);
#endif  // #ifdef WOM_ATTACH_ISR

    // Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    // set up mpu6886 for low-power operation
    M5.Imu.Init();  // basic init
    M5.Imu.enableWakeOnMotion(M5.Imu.AFS_16G, 10);

    // wait until IMU ISR hasn't triggered for X milliseconds
    while (1) {
        uint32_t since_last_wom_millis = millis() - g_wom_last_millis;
        if (since_last_wom_millis > 5000) {
            break;
        }
        Serial.printf("waiting:%dms, ", since_last_wom_millis);
        delay(1000);
    }

    // disable all wakeup sources
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // enable waking up on pin 35 (from IMU)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);  // 1 = High, 0 = Low

    // Go to sleep now
    Serial.printf("\nGoing to sleep now\n");
    M5.Axp.SetSleep();  // conveniently turn off screen, etc.
    delay(100);
    esp_deep_sleep_start();
    Serial.printf("\nThis will never be printed");
}

void loop() {
}