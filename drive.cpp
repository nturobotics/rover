#include <WiFi.h>

const char *ssid = "";
const char *password = "";

// Motor pins
#define IN1 13
#define IN2 14
#define IN3 2
#define IN4 4

#define PWM_FREQ 5000
#define PWM_RES 8   // 0-255

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Attach each pin to its own PWM channel (automatically assigned)
  // ledcAttach(pin, freq, resolution) - available in older and newer cores
  ledcAttach(IN1, PWM_FREQ, PWM_RES);
  ledcAttach(IN2, PWM_FREQ, PWM_RES);
  ledcAttach(IN3, PWM_FREQ, PWM_RES);
  ledcAttach(IN4, PWM_FREQ, PWM_RES);
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
  ledcWrite(IN3, 0);
  ledcWrite(IN4, 0);
}

void turnRight(int speed) {
  ledcWrite(IN1, speed);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, speed);
}

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  setupMotors();
  stopMotors();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\http://" + WiFi.localIP().toString() + "/");
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  int speed = 200;  // adjust as needed

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      currentLine += c;

      if (c == '\n' && currentLine.length() == 2) {
        // send HTML
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

      if (currentLine.endsWith("GET /F")) moveForward(speed);
      else if (currentLine.endsWith("GET /B")) moveBackward(speed);
      else if (currentLine.endsWith("GET /L")) turnLeft(speed);
      else if (currentLine.endsWith("GET /R")) turnRight(speed);
      else if (currentLine.endsWith("GET /S")) stopMotors();

      if (c == '\n') currentLine = "";
    }
  }
  client.stop();
}
