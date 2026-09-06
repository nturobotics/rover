#include <WiFi.h>

const char *ssid = "{wifi}";
const char *password = "{password}";

// ===== Motor Pins (safe pins) =====
#define IN1 13
#define IN2 14
#define IN3 15   // use GPIO15 (add 10k pulldown) or GPIO2 (also needs pulldown)
#define IN4 16   // GPIO16 is safe

#define PWM_FREQ 5000
#define PWM_RES 8   // 0-255

WiFiServer server(80);

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Attach all pins to PWM (duty cycle defaults to 0)
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
  setupMotors();
  stopMotors();  // ensure motors are off at start

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP: " + WiFi.localIP().toString());
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  int speed = 200;  // adjust 0-255

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      currentLine += c;

      // Detect end of HTTP request (blank line)
      if (c == '\n' && currentLine.length() == 2) {
        // Send HTML page
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

      // ===== FIXED: Use startsWith() to catch the full request =====
      if (currentLine.startsWith("GET /F")) {
        moveForward(speed);
      } else if (currentLine.startsWith("GET /B")) {
        moveBackward(speed);
      } else if (currentLine.startsWith("GET /L")) {
        turnLeft(speed);
      } else if (currentLine.startsWith("GET /R")) {
        turnRight(speed);
      } else if (currentLine.startsWith("GET /S")) {
        stopMotors();
      }

      // Reset line buffer after each newline
      if (c == '\n') currentLine = "";
    }
  }
  client.stop();
}
