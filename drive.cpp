#include <WiFi.h>

const char *ssid = "12";
const char *password = "12121212";

// ===== Motor pins =====
#define IN1 13
#define IN2 14
#define IN3 15   // ADD a 10k resistor from GPIO15 to GND !
#define IN4 4    // Flash LED – disable flash if used

#define PWM_FREQ 5000
#define PWM_RES 8

WiFiServer server(80);

void setupMotors() {
  // ===== Step 1: Digital LOW before anything else =====
  pinMode(IN1, OUTPUT);
  digitalWrite(IN1, LOW);
  
  pinMode(IN2, OUTPUT);
  digitalWrite(IN2, LOW);
  
  pinMode(IN3, OUTPUT);
  digitalWrite(IN3, LOW);
  
  pinMode(IN4, OUTPUT);
  digitalWrite(IN4, LOW);

  // ===== Step 2: Attach PWM and clear duty =====
  ledcAttach(IN1, PWM_FREQ, PWM_RES);
  ledcWrite(IN1, 0);
  
  ledcAttach(IN2, PWM_FREQ, PWM_RES);
  ledcWrite(IN2, 0);
  
  ledcAttach(IN3, PWM_FREQ, PWM_RES);
  ledcWrite(IN3, 0);
  
  ledcAttach(IN4, PWM_FREQ, PWM_RES);
  ledcWrite(IN4, 0);

  // Final safety stop
  stopMotors();
}

void stopMotors() {
  ledcWrite(IN1, 0);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, 0);
}

void moveForward(int speed) {
  ledcWrite(IN1, speed);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, speed);
  ledcWrite(IN4, 0);
}

void moveBackward(int speed) {
  ledcWrite(IN1, 0);
  ledcWrite(IN2, speed);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, speed);
}

void turnLeft(int speed) {
  ledcWrite(IN1, 0);
  ledcWrite(IN2, speed);
  ledcWrite(IN3, speed);
  ledcWrite(IN4, 0);
}

void turnRight(int speed) {
  ledcWrite(IN1, speed);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, speed);
}

void setup() {
  Serial.begin(115200);
  delay(1000);      // Let everything settle
  setupMotors();    // Motors will be OFF

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nhttp://" + WiFi.localIP().toString() + "/");
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  int speed = 200;  // Adjust as needed

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();

      if (c == '\n') {
        if (currentLine.length() == 0 || currentLine == "\r") {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<html><body>");
          client.println("<a href='/F'><button>Forward</button></a><br>");
          client.println("<a href='/B'><button>Backward</button></a><br>");
          client.println("<a href='/L'><button>Left</button></a><br>");
          client.println("<a href='/R'><button>Right</button></a><br>");
          client.println("<a href='/S'><button>STOP</button></a><br>");
          client.println("</body></html>");
          break;
        }

        if (currentLine.indexOf("GET /F") >= 0) moveForward(speed);
        else if (currentLine.indexOf("GET /B") >= 0) moveBackward(speed);
        else if (currentLine.indexOf("GET /L") >= 0) turnLeft(speed);
        else if (currentLine.indexOf("GET /R") >= 0) turnRight(speed);
        else if (currentLine.indexOf("GET /S") >= 0) stopMotors();

        currentLine = "";
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  client.stop();
}
