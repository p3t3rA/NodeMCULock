#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>

Servo servo;
WiFiServer server(80);
const char* ssid = "wifi";
const char* password = "pass";
int ledPin = 13; // GPIO13
int CASEOPEN = 100;
int CASECLOSE = 10;
bool CASECLOSESTATE = true;
String Pin;
int timermin = 60;
bool timeractive = false;
int timer = 0;
String randnumber;
int switchPin = 4;
void setup() {
  delay(5); //Condensator need time to load
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT);
  
  server.begin();
  servo.attach(2);
  EEPROM.begin(512);
  for (int i = 0; i < 4; ++i)
  {
    Pin += char(EEPROM.read(i));
    Serial.print(char(EEPROM.read(i)));
  }
  if (EEPROM.read(5) == 1) {
    CASECLOSESTATE = true;
    digitalWrite(ledPin, HIGH);
    servo.write(CASECLOSE);
  } else {
    CASECLOSESTATE = false;
    digitalWrite(ledPin, LOW);
    servo.write(CASEOPEN);
  }
  String oldRandNumber = "";
  for (int i = 0; i < 5; ++i)
  {
    oldRandNumber += char(EEPROM.read(i + 6));
    Serial.print(char(EEPROM.read(i + 6)));
  }
  randnumber = oldRandNumber;
  Serial.println(Pin);
  Serial.println("\nPIN:" + Pin + "!");
  Serial.println("Randnumber: " + randnumber);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi Started");
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  delay(1000);
}

void loop() {
  delay(1000);
  if (timeractive) {
    timerHandler();
  }
  if (WiFi.status() == 6)
  {
    Serial.println("RESET!!!!!!!!!!!!!!!!!!!!!!");
    ESP.reset();
  }
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
    if (CASECLOSESTATE) {
      value = getCloseCase();
      value = "Already locked!";
    } else {
      value = getCloseCase();
      writeRandomNumber();
    }
  } else if (request.indexOf("/ChangePin")  != -1) {
    value = getChangePin();
  } else if (request.indexOf("/open")  != -1) {
    if (CASECLOSESTATE) {
      value = openCase(request);
    } else {
      value = openCase(request);
      value = "Already Open!";      
    }
  } else if (request.indexOf("/CloseTime") != -1) {
    value = closeTime(request);
  } else if (request.indexOf("/newpin")  != -1) {
    value = setPin(request);
  }
  SendAnswer(client, value);
}

void timerHandler() {
  if (timermin == 0 ) {
    timermin = 60;
    if ( timer > 0 ) {
      timer--;
    } else {
      timeractive = false;
      OpenCaseDo();
      CASECLOSESTATE = false;
    }
  } else {
    timermin--;
  }
}

void SendAnswer(WiFiClient client, String value) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println(getStatus());
  client.println(value);
  client.println("<br><br>");
  client.println(getStartPage());
  if (digitalRead(switchPin)) {
    client.println("<br>Deckel ist Offen!");
  } else {
    client.println("<br>Deckel ist geschlossen!");
  }
  client.println("<p>Zufallsnummer des letzten Zuschlusses: '" + randnumber + "'. Wird nur bei Verschluss gewechselt!");
  client.println("</html>");
}

String closeTime(String request) {
  return request;
}

String getStatus() {
  if (CASECLOSESTATE) {
    return ("<h2>Status: Geschlossen!</h2>");
  } else {
    return ("<h2>Status: Offen!</h2>");
  }
}

String setPin(String request) {
  String body = splitGetSecond("?", request);
  String newpin = splitGetSecond("=", splitGetFirst("&", body));
  String oldpin = splitGetSecond("=", splitGetSecond("&", splitGetFirst(" ", body)));
  if (oldpin == Pin ) {
    EEPROM.begin(512);
    for (int i = 0; i < newpin.length(); ++i)
    {
      EEPROM.write(i, newpin[i]);
    }
    EEPROM.commit();
    Pin = newpin;
    return "PIN Saved: " + Pin + "!";
  } else {
    return "Wrong PIN!";
  }
}

String getOpenCase() {
  return ("<form action='/open' method='get'>Pin eingeben:<br><input type='text' name='pin'><input type='submit' value='Submit'></form>");
}
String getCloseCase() {
  CloseCase();
  CASECLOSESTATE = true;
  EEPROM.begin(512);
  EEPROM.write(5, 1);
  EEPROM.commit();
  return "Case Closed";
}
String getChangePin() {
  return "<form action='/newpin' method='get'>Neuen Pin eingeben:<br><input type='text' name='newpin'><br>Alten Pin eingeben:<br><input type='text' name='oldpin'><input type='submit' value='Speichern'></form>";
}
String getStartPage() {
  return ("<p>Wilkommen!</p><a href='/OpenCase'><button>Oeffnen</button></a></br><a href='/CloseCase'><button>Schliessen</button><br><a href='/CloseTime'><button>Close for time</button></a>Unfinished!</br><a href='/ChangePin'><button>Pin aendern</button></a></br>");
}
String openCase(String request) {
  String PinToCheck = splitGetSecond("=", splitGetFirst(" ", splitGetSecond("?", request)));
  if (checkPin(PinToCheck)) {
    OpenCaseDo();
    CASECLOSESTATE = false;
    EEPROM.begin(512);
    EEPROM.write(5, 0);
    EEPROM.commit();
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

void OpenCaseDo() {
  servo.attach(2);
  servo.write(CASEOPEN);
  servo.write(CASEOPEN);
  servo.write(CASEOPEN);
  servo.write(CASEOPEN);
  delay(500);
  digitalWrite(ledPin, LOW);
  CASECLOSESTATE = false;
}
void CloseCase() {
  digitalWrite(ledPin, HIGH);
  servo.attach(2);
  servo.write(CASECLOSE);
  delay(500);
  CASECLOSESTATE = true;
  
}
void writeRandomNumber() {
randnumber = random(10000, 99999);
  Serial.println(randnumber);
  EEPROM.begin(512);
  for (int i = 0; i < randnumber.length(); ++i)
  {
    EEPROM.write(i + 6, randnumber[i]);
  }
  EEPROM.commit();
}

