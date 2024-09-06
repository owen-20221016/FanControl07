#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
// #include <Wire.h>
#include <ESPAsyncWebServer.h>
#include <string.h>
#include <ArduinoJson.h>
#include "X9C10X.h"

AsyncWebServer server(80);
JsonDocument status;

X9C10X pot(45800);
const char* htmlPage = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0, minimum-scale=0.5, maximum-scale=2.0, user-scalable=yes'/><meta http-equiv='X-UA-Compatible'content='ie=edge'><title>Fan Control System</title><style>h2,h1{line-height:1%}body{margin:0;padding:0;background:LightCyan}.button1{width:150px;height:100px;text-align:center;font-weight:100;color:darkcyan;margin:0 40px 35px 0;position:relative;border:none;background:LightCyan;font-size:20px;border-radius:10%;outline:none}.button{width:100px;height:100px;text-align:center;font-weight:100;color:darkcyan;margin:0 40px 35px 0;position:relative;border:6px solid darkcyan;background:LightCyan;font-size:20px;border-radius:50%;outline:none}.button.on{color:Lime;background:DarkTurquoise}.button.off{color:red;background:DarkTurquoise}</style></head><body><div class='tabs'><section class='cont'style='display:block'><ol><center><span style='left: -20px; position: relative;'id='time1'>Time Loading......</span><script>setInterval('time1.innerHTML=new Date().toLocaleString()',1000);</script></center><center></br><button id='switch'type='button'class='button'onclick='confirmSwitch()'>Switch</button></br><button id='speed'type='button'class='button1'>Speed:0</button></br><button type='button'class='button'onclick='checkSwitchWhenChangeSpeed(`up`)'>Speed Up</button><button type='button'class='button'onclick='checkSwitchWhenChangeSpeed(`down`)'>Speed Down</button></br></center><script>var position=0;var status='false';function confirmSwitch(){if(confirm('是否操作开关？')){fetchURL('/switch',`status=${status=='true'?'off':'on'}`)}};function checkSwitchWhenChangeSpeed(action){if(status=='true'){fetchURL('/changeSpeed',`changeSpeed=${action}`)}else if(confirm('开关未开启，是否开启？')){fetchURL('/switch','status=on')}};function fetchURL(url,data){var xhr;xhr=new XMLHttpRequest();xhr.open('POST',url,true);xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');xhr.onreadystatechange=function(){if(xhr.readyState==4&&xhr.status==200){const obj=JSON.parse(xhr.responseText);position=obj.position;status=obj.status;var switchDoc=document.getElementById('switch');var speedDoc=document.getElementById('speed');if(status=='true'){switchDoc.classList.remove('off');switchDoc.classList.add('on')}else{switchDoc.classList.remove('on');switchDoc.classList.add('off')}speedDoc.innerText=`Speed:${position}`}};xhr.send(data)};window.onload=function(){fetchURL('/getStatus',null)};</script></body></html>";

const char* ssid = "";
const char* password = "";

String getStatus() {
  String t = "";
  serializeJson(status,t);
  return t;
}

void changePosition(bool uper) {
  int position = status["position"];
  if (uper) {
    for(int i = 0; i < 10; i++ ) {
      pot.incr();
      position += 1;
      delay(100);
    };
    } else {
      for(int i = 0; i < 10; i++ ) {
        pot.decr();
        position -= 1;
        delay(100);
    };
  };
  status["position"] = position;
};

void setup(){
  Serial.begin(115200);
  
  pot.begin(D5, D2, D1);  //  pulse, direction, select //inc, u/d , cs 
  pot.setPosition(0,true);
  pinMode(D6, OUTPUT); // switch output

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
  }
  Serial.printf("WiFi Connected!\n");
  Serial.println(WiFi.localIP());
  Serial.printf("debug v0.1");
  status["status"] = false;
  status["position"] = 0;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("visit index");
    request->send_P(200, "text/html", htmlPage);
  });

  server.on("/getStatus", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("get status");
    request->send(200,"application/json", getStatus());
  });

  server.on("/changeSpeed", HTTP_POST, [](AsyncWebServerRequest *request){
    String action = request->getParam("changeSpeed", true, false)->value();
    Serial.println("action change speed:" + action);
    if (!action.compareTo("up") && status["position"] <= 90) {
      changePosition(true);
    } else if (!action.compareTo("down") && status["position"] >= 10) {
      changePosition(false);
    };
    request->send(200, "application/json", getStatus());
  });

  server.on("/switch",HTTP_POST,[](AsyncWebServerRequest *request) {
    String action = request->getParam("status", true, false)->value();
    Serial.println("action switch:" + action);
    bool s = status["status"];
    if (!action.compareTo("on")) {
      digitalWrite(D6,HIGH);
      s = true;
    } else {
      digitalWrite(D6,LOW);
      s = false;
    };
    status["status"] = s;
    request->send(200, "application/json", getStatus());
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/");
  });

  server.begin();
}

void loop(){

}
