
#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
WiFiClient client;
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=tokyo,jp&APPID=";
const String key = "weather_api_key";
// Wifi接続情報
char ssid[] = "wifi_ssid";
char password[] = "wifi_pass";
#define PIR_MOTION_SENSOR 2
unsigned long lastTime;
char *previous_weather;
double previous_temp;
// Wifiをセットアップする
void setupWifi()
{
    Serial.printf("WIFI: Connecting to %s ", ssid);
    // Wifi接続を開始する
    WiFi.begin(ssid, password);
    // Wifi接続まで待つ
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);
    }
    Serial.println();
    // 接続完了
    Serial.printf("WIFI :STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}
void displayWeather()
{
    // 天気をLCDに表示
    M5.Lcd.setCursor(20, 40);
    M5.Lcd.setTextSize(6);
    M5.Lcd.print(previous_weather);
    // 気温をLCDに表示
    M5.Lcd.setCursor(20, 150);
    M5.Lcd.setTextSize(6);
    M5.Lcd.printf("%.1f", previous_temp);
}
bool requestWeather()
{
    bool result = false;
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http_client;
        // URLを指定
        http_client.begin(endpoint + key);
        // GETリクエストを送信
        int http_code = http_client.GET();
        if (http_code > 0)
        {
            // レスポンス（JSON）を取得
            String json = http_client.getString();
            StaticJsonDocument<2000> json_buffer;
            auto error = deserializeJson(json_buffer, json);
            if (error)
            {
                Serial.print("deserializeJson() failed.");
                Serial.println(error.c_str());
            }
            else
            {
                // JSONから天気と気温情報を取り出す
                JsonObject json_object = json_buffer.as<JsonObject>();
                previous_weather = (char *)json_object["weather"][0]["main"].as<char *>();
                previous_temp = json_object["main"]["temp"].as<double>() - 273.15;
                // 天気情報を取得した時間を保存
                lastTime = millis();
                result = true;
            }
        }
        else
        {
            // Wifi接続に失敗した場合
            Serial.println("Error on http request");
        }
        // リソースを解放
        http_client.end();
    }
    return result;
}
void setup()
{
    // M5Stackオブジェクトを初期化する
    M5.begin();
    M5.Lcd.print("Waiting...");
    // LCDの色設定
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    // Wifiに接続する
    setupWifi();
    // デジタルピン 2を入力に設定
    pinMode(PIR_MOTION_SENSOR, INPUT);
    // Deep Sleepからの復帰条件を設定。GPIO PIR_MOTION_SENSORに入力があると起動
    M5.setWakeupButton(PIR_MOTION_SENSOR);
    // 初回起動時に天気を取得してディスプレイに情報を表示する
    bool result = requestWeather();
    if (result)
    {
        displayWeather();
        delay(5 * 1000);
        M5.Lcd.clear();
    }
}
void loop()
{
    // 人感センサーがHIGHのときは天気情報取得処理を動かす
    if (digitalRead(PIR_MOTION_SENSOR))
    {
        unsigned long currentTime = millis();
        unsigned long thirtyMinutes = 1000 * 60 * 30;
        // 前回のデータ取得から30分経過していた場合は、天気情報を再取得する
        if (thirtyMinutes < (currentTime - lastTime))
        {
            bool result = requestWeather();
            if (result)
            {
                // APIから受け取ったデータでLCDの情報を更新
                displayWeather();
            }
        }
        else
        {
            // 30経過していない場合は、キャッシュの情報をLCDに表示
            displayWeather();
        }
        // 10秒経過したら画面の表示情報をクリアする
        delay(10 * 1000);
        M5.Lcd.clear();
    }
    else
    {
        // 1時間
        unsigned long oneHour = 1000 * 60 * 60 * 1;
        unsigned long currentTime = millis();
        if (oneHour < (currentTime - lastTime))
        {
            // 人感センサーがLOWで、かつ1時間起動がなかったら、節電のためM5StackをDeep Sleepに切り替えます
            M5.powerOFF();
        }
    }
    delay(1 * 1000);
    M5.update();
}