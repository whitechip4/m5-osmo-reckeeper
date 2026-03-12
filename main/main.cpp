#include <M5Unified.h>
#include <esp_log.h>

static const char* TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "M5StickC Plus2 Osmo Rekeeper starting...");

    M5.begin();

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextDatum(middle_center);

    M5.Lcd.drawString("Osmo Rekeeper", M5.Lcd.width() / 2, M5.Lcd.height() / 2 - 20);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("LCD Test OK", M5.Lcd.width() / 2, M5.Lcd.height() / 2 + 20);

    ESP_LOGI(TAG, "LCD display initialized");

    while (true) {
        M5.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
