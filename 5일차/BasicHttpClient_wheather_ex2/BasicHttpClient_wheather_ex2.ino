#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#define USE_SERIAL Serial

WiFiMulti wifiMulti;

int segmentLEDs[] = {26,25,17,16,27,14,12,13};
int digit_select_num[] = {5,23,19,18};
int digitForNum[10][8] = {
  {1, 1, 1, 1, 1, 1, 0, 0}, //0
  {0, 1, 1, 0, 0, 0, 0, 0}, //1
  {1, 1, 0, 1, 1, 0, 1, 0}, //2
  {1, 1, 1, 1, 0, 0, 1, 0}, //3
  {0, 1, 1, 0, 0, 1, 1, 0}, //4
  {1, 0, 1, 1, 0, 1, 1, 0}, //5
  {1, 0, 1, 1, 1, 1, 1, 0}, //6
  {1, 1, 1, 0, 0, 0, 0, 0}, //7
  {1, 1, 1, 1, 1, 1, 1, 0}, //8
  {1, 1, 1, 1, 0, 1, 1, 0}  //9
};
int tones[8] = {261, 293, 329, 349, 392,440,494,523};
int echoPin = SDA;
int trigPin = SCL;
void setup() {

    USE_SERIAL.begin(115200);

    for(int i = 0 ; i < 8 ; i++) {
      pinMode(segmentLEDs[i], OUTPUT);
    }
    for(int i=0; i<4;i++)
    {
      pinMode(digit_select_num[i], OUTPUT);
    }
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    for(uint8_t t = 2; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }
    ledcSetup(0,1000,10);
    ledcAttachPin(2,0);
    wifiMulti.addAP("mesmo 2G", "13245687");
    xTaskCreate(task1,"task1",2048,NULL,2,NULL);
    xTaskCreate(task2,"task2",2048,NULL,2,NULL);
}
String predictTime;
String remain_seat;
float distance = 0;
int endtag_index = 0;
String tag_str;
String content_str;
void loop() {

  
  // wait for WiFi connection
  if((wifiMulti.run() == WL_CONNECTED)) {

      HTTPClient http;

      USE_SERIAL.print("[HTTP] begin...\n");
      // configure traged server and url
      http.begin("http://openapi.gbis.go.kr/ws/rest/busarrivalservice/station?serviceKey=8qX8JLHMKFXxDq8aenURiQ7CX5wzFGQ544rVTTmC8%2BFDqgJ8PM%2B923irQiS0CnNhV0exuv640M3cBnElawGPjg%3D%3D&stationId=200000078&"); //HTTP

      USE_SERIAL.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              //USE_SERIAL.println(payload);
              endtag_index = payload.indexOf("</busArrivalList>");
              //USE_SERIAL.println(endtag_index);

              if(endtag_index > 0)
              {
                  tag_str = "<busArrivalList>";
                  content_str =payload.substring(payload.indexOf(tag_str)+tag_str.length(),endtag_index);
                  //USE_SERIAL.println(content_str);

                  endtag_index = content_str.indexOf("</remainSeatCnt1>");
                  if(endtag_index > 0)
                  {
                    tag_str = "<remainSeatCnt1>";
                    remain_seat =content_str.substring(content_str.indexOf(tag_str)+tag_str.length(),endtag_index);
                    USE_SERIAL.println(remain_seat);
                  }

                  endtag_index = content_str.indexOf("</predictTime1>");
                  if(endtag_index > 0)
                  {
                    tag_str = "<predictTime1>";
                    predictTime =content_str.substring(content_str.indexOf(tag_str)+tag_str.length(),endtag_index);
                    USE_SERIAL.println(predictTime);
                  }
              }
          }
      } else {
          USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
  }
  vTaskDelay(30000/portTICK_PERIOD_MS);
}

int display_number;
bool first_flag = false;
void task1(void *parameter)
{
  while(1){
      if(predictTime.toInt() > 0)
      {
       display_number = (predictTime.toInt() * 100)+remain_seat.toInt();
       seg_display(display_number);
      }
  }
}

void task2(void *parameter)
{
  while(1){
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
  
    unsigned long duration = pulseIn(echoPin, HIGH);
    distance = ((float)(340*duration)/1000)/2;
  
    Serial.print(distance);
    Serial.println("cm");

    if((distance < 1000) && (predictTime.toInt() < 5))
    {
      ledcWrite(0,tones[0]);
      vTaskDelay(500/portTICK_PERIOD_MS);
      ledcWrite(0,tones[3]);
      vTaskDelay(500/portTICK_PERIOD_MS);
      ledcWrite(0,tones[6]);
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
    else
    {
      ledcWrite(0,0);
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void seg_display(int number) //1234 , 4321
{
  int digit4 = number/1000;
  int digit3 = (number%1000)/100;
  int digit2 = (number%100)/10;
  int digit1 = (number%10);

  seg_1_digit(1,digit4);
  vTaskDelay(2/portTICK_PERIOD_MS);
  seg_1_digit(2,digit3);
  vTaskDelay(2/portTICK_PERIOD_MS);
  seg_1_digit(3,digit2);
  vTaskDelay(2/portTICK_PERIOD_MS);
  seg_1_digit(4,digit1);
  vTaskDelay(2/portTICK_PERIOD_MS);
}

void seg_1_digit(int digit, int num)
{   switch(digit)
    {
      case 1:
        digitalWrite(digit_select_num[0], LOW);
        digitalWrite(digit_select_num[1], HIGH);
        digitalWrite(digit_select_num[2], HIGH);
        digitalWrite(digit_select_num[3], HIGH);
        break;
      case 2:
        digitalWrite(digit_select_num[0], HIGH);
        digitalWrite(digit_select_num[1], LOW);
        digitalWrite(digit_select_num[2], HIGH);
        digitalWrite(digit_select_num[3], HIGH);
        break;
      case 3:
        digitalWrite(digit_select_num[0], HIGH);
        digitalWrite(digit_select_num[1], HIGH);
        digitalWrite(digit_select_num[2], LOW);
        digitalWrite(digit_select_num[3], HIGH);
        break;
      case 4:
        digitalWrite(digit_select_num[0], HIGH);
        digitalWrite(digit_select_num[1], HIGH);
        digitalWrite(digit_select_num[2], HIGH);
        digitalWrite(digit_select_num[3], LOW);
        break;
    }
    for(int j=0; j< 8;j++){
      digitalWrite(segmentLEDs[j],digitForNum[num][j]);
    }
}
