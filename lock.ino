#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <EEPROM.h>


#define OLED_RESET LED_BUILTIN  //4
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Servo servo;
WiFiServer server(80);
const char* ssid = "meinwifi2";
const char* password = "happybanana636";
int ledPin = 13; // GPIO13
int CASEOPEN = 180;
int CASECLOSE = 10;
String Pin = "1234";

void setup() {
  Serial.begin(115200);

  EEPROM.begin(512);
  Pin = "";
  for (int i = 0; i < 4; ++i)
  {
    Pin += char(EEPROM.read(i));
    Serial.print(char(EEPROM.read(i)));
  }

  Serial.println("\nPIN:" + Pin + "!");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi Started");

  server.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  displayText("Case Open\nStartup\nPIN: " + Pin);
  servo.attach(2); //D4
  servo.write(CASEOPEN);
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  delay(1000);
}

void loop() {
  delay(1000);
  Serial.println("Loop");
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }

  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  String value = "";
  if (request.indexOf("/CaseOpen") != -1)  {
    if ( Pin.length() > 0 ) {
      String NewPin = SplitString(request);
      if (Pin == NewPin) {
        value = OpenCase();
      } else {
        value = "Falscher PIN!";
      }
    }
  } else if (request.indexOf("/CaseLocked") != -1)  {
    value = CloseCase();
  } else if (request.indexOf("/NewPin") != -1) {
    String NewPin = SplitString(request);
    String OldPin = SplitString2(request);
    if ( Pin == OldPin ) {
      Pin = NewPin;
      displayText("New PIN: " + Pin);
      value = "New PIN Set: " + Pin;
      EEPROM.begin(512);
      for (int i = 0; i < Pin.length(); ++i)
      {
        EEPROM.write(i, Pin[i]);
        Serial.print(Pin[i]);
      }
      EEPROM.commit();
    } else {
      value = "Falscher Pin!";
    }
  }
  SendAnswer(client, value);
  delay(1);

}


String SplitString(String input) {
  String value1, value2;
  for (int i = 0; i < input.length(); i++) {
    if (input.substring(i, i + 1) == ",") {
      value1 = input.substring(0, i);
      value2 = input.substring(i + 1);
      break;
    }
  }
  return (SplitString2(value2));
}
String SplitString2(String input) {
  String value1, value2;
  for (int i = 0; i < input.length(); i++) {
    if (input.substring(i, i + 1) == ",") {
      value1 = input.substring(0, i);
      value2 = input.substring(i + 1);
      break;
    }
  }
  return (value1);
}


void displayText(String text) {
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.display();
}

void SendAnswer(WiFiClient client, String value) {

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>Case is now: ");
  client.println(value);
  client.println("<br><br>");
  client.println("<a href=\"/CaseOpen\"\"><button>Open the Case </button></a>");
  client.println("<a href=\"/CaseLocked\"\"><button>Lock Case </button></a><br />");
  client.println("</html>");
}

String OpenCase() {
  int value = LOW;
  digitalWrite(ledPin, LOW);
  displayText("Case Open");
  value = LOW;
  servo.attach(2);
  servo.write(CASEOPEN);
  return ("Open");
}
String CloseCase() {
  int value = LOW;
  digitalWrite(ledPin, HIGH);
  value = HIGH;
  displayText("Close Case");
  servo.attach(2);
  servo.write(CASECLOSE);
  return ("Locked");
}
