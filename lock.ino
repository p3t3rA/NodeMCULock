#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>

Servo servo;
WiFiServer server(80);
const char* ssid = "SSID";
const char* password = "PASSWORD";
int ledPin = 13; // GPIO13
int CASEOPEN = 180;
int CASECLOSE = 10;
bool CASECLOSESTATE = true;
String Pin = "1234";

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  for (int i = 0; i < 4; ++i)
  {
    Pin += char(EEPROM.read(i));
    Serial.print(char(EEPROM.read(i)));
  }

  Serial.println("\nPIN:" + Pin + "!");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi Started");

  server.begin();
  servo.attach(2); //D4
  servo.write(CASECLOSE);
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

  client.flush();

  String value = "";
  if (request.indexOf("/OpenCase") != -1)  {
    value = getOpenCase();
  } else if (request.indexOf("/CloseCase")  != -1) {
    value = getCloseCase();
  } else if (request.indexOf("/ChangePin")  != -1) {
    value = getChangePin();
  } else if (request.indexOf("/open")  != -1) {
    value = openCase(request);
  }
  SendAnswer(client, value);
  delay(1);

}

void SendAnswer(WiFiClient client, String value) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println(value);
  client.println("<br><br>");
  client.println(getStartPage());
  client.println("</html>");
}

String getOpenCase() {
  return ("<form action='/open' method='get'>Enter Pin:<br><input type='text' name='pin'><input type='submit' value='Submit'></form>");
}
String getCloseCase() {
  CloseCase();
  return "Case Closed";
}
String getChangePin() {
  return "Pin";
}
String getStartPage() {
  return ("<p>Wilkommen!</p><a href='/OpenCase'><button>Open the Case </button></a></br><a href='/CloseCase'><button>Close the Case </button></a></br><a href='/ChangePin'><button>Change Pin</button></a></br>");
}
String openCase(String request) {
  String PinToCheck = splitGetSecond("=", splitGetFirst(" ", splitGetSecond("?", request)));
  if (checkPin(PinToCheck)) {
    OpenCase();
    return "OK!";
  } else {
    return "falscher PIN!";
  }
}

bool checkPin(String PinToCheck) {
  if (Pin == PinToCheck ) {
    return true;
  } else {
    return false;
  }
}

String splitGetFirst(String separator, String input) {
  String value1;
  Serial.println(input);
  for (int i = 0; i < input.length(); i++) {
    if (input.substring(i, i + 1) == separator) {
      value1 = input.substring(0, i);
      break;
    }
  }
  return value1;
}

String splitGetSecond(String separator, String input) {
  String value2;
  Serial.println(input);
  for (int i = 0; i < input.length(); i++) {
    if (input.substring(i, i + 1) == separator) {
      value2 = input.substring(i + 1);
      break;
    }
  }
  return value2;
}

void OpenCase() {
  digitalWrite(ledPin, LOW);
  servo.attach(2);
  servo.write(CASEOPEN);
  CASECLOSESTATE = false;
}
void CloseCase() {
  digitalWrite(ledPin, HIGH);
  servo.attach(2);
  servo.write(CASECLOSE);
  CASECLOSESTATE = true;
}
