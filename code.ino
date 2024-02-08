#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define dhtPin 32
#define flamePin 25
#define MQ_PIN 34
#define dhtType DHT22
#define API_key "";         // Project Setting/Web API Key
#define DbUrl "";          // Realtime Db = https://.......asia-southeast1.firebasedatabase.app/
#define ssid "Zy"         // SSID Wifi
#define pass "password2" // Pw nya
#define RL_VALUE 5
#define RO_CLEAN_AIR_FACTOR 9.83
#define CALIBARAION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 500
#define READ_SAMPLE_INTERVAL 50
#define READ_SAMPLE_TIMES 5

#define BUZZER_PIN 12  

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool buzzerActive = false;

//float Ro = 10;
String flameStatus;
unsigned long prevMillis = 0;
bool signUpOk = false;
DHT dht(dhtPin, dhtType);

int smokeValue=0;

void setup()
{
  Serial.begin(115200);
  pinMode(flamePin, INPUT);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  dht.begin();

  config.api_key = API_key;
  config.database_url = DbUrl;

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("LoginOk");
    signUpOk = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.print("Sabar...\n");

  pinMode(BUZZER_PIN, OUTPUT);
  smokeValue = 0;
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (Firebase.ready() && signUpOk && (millis() - prevMillis > 100 || prevMillis == 0))
  {
    Serial.print("ADc :");
    Serial.println(analogRead(MQ_PIN));
    prevMillis = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int                                                                                                                                                                                                                                                                                                                    flame = digitalRead(flamePin);

    smokeValue = analogRead(MQ_PIN);
    smokeValue = map(smokeValue, 0, 4095, 0, 100);


//    Serial.print(gas);
    Serial.println("ppm");

//    API STATUS
    if (flame == LOW)
    {
      flameStatus = "On"; 
    }
    else
    {
      flameStatus = "Off";
    }
    Serial.println(h);
    Serial.println(t);

//    SUHU
    if (Firebase.RTDB.setFloat(&fbdo, "detect/Suhu", t))
    {
      Serial.println("Data suhu terkirim");
      if (t >= 30)
      {
        activateAlarm();
        Serial.println("ALARM: Suhu tinggi terdeteksi!");
      }
      else
      {
        deactivateAlarm();
      }
    }
    else
    {
      Serial.println("Data Suhu gagal terkirim");
    }
    
//  Kelembaban  
    if (Firebase.RTDB.setFloat(&fbdo, "detect/Kelembaban", h))
    {
      Serial.println("Data kelembaban terkirim");
    }
    else
    {
      Serial.println("Data kelembaban gagal terkirim");
    }
     // ASAP
    if (Firebase.RTDB.setInt(&fbdo, "detect/Asap", smokeValue)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      if (smokeValue >= 50) {
        activateAlarm();
        Serial.println("ALARM: Nilai asap tinggi terdeteksi!");
      } else {
        
        deactivateAlarm();
      }
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

//    Api
    if (Firebase.RTDB.setString(&fbdo, "detect/Flame", flameStatus))
    {
      Serial.println("Data flame terkirim");
      if (flameStatus == "On")
      {
        activateAlarm();
      }
      else
      {
        deactivateAlarm();
      }
    }
    else
    {
      Serial.println("Data flame gagal terkirim");
    }

  }
}
// 

void activateAlarm()
{
  if (!buzzerActive )
  {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerActive = true;
    delay(3000);
  }
}

void deactivateAlarm()
{
  if (buzzerActive)
  {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}
