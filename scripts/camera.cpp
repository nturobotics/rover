/*
 * ESP32-CAM (AI-Thinker) — Car Control + Live Video Stream
 * ---------------------------------------------------------
 * Combines:
 *   1) The official CameraWebServer camera-init logic
 *      (PSRAM check, sensor tuning, etc.)
 *   2) L298N 4-motor control (forward/back/left/right/stop + per-motor test)
 * ...into a SINGLE WebServer on port 80, with a live MJPEG stream
 * embedded directly in the control page.
 *
 * BOARD: "AI Thinker ESP32-CAM"
 * Tools > Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
 * Tools > PSRAM: "Enabled"
 *
 * IMPORTANT WIRING NOTE:
 * The AI-Thinker board's microSD card shares GPIO 2, 12, 13, 14, 15
 * with the camera module. Since this sketch does NOT use the SD card,
 * pins 2, 13, 14, 15 are safely reused here to drive the L298N.
 * Do not insert an SD card / do not call SD_MMC.begin() in this sketch.
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ===================== WiFi credentials =====================
const char *ssid     = "12";
const char *password = "12121212";

// ===================== L298N Motor Pins =====================
#define IN1 2   // Left motor  +
#define IN2 14  // Left motor  -
#define IN3 15  // Right motor +
#define IN4 13  // Right motor -

// ===================== AI-Thinker Camera Pins =====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

// ===================== Motor Functions =====================
void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  Serial.println("Motor: Forward");
}

void moveBackward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  Serial.println("Motor: Backward");
}

void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
  Serial.println("Motor: Left");
}

void turnRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  Serial.println("Motor: Right");
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  delay(10);  // let the L298N settle
  Serial.println("Motor: Stop");
}

void testMotor(int motor, bool forward) {
  // motor: 0 = left, 1 = right
  if (motor == 0) {
    if (forward) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
    else         { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  } else {
    if (forward) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
    else         { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  }
}

// ===================== Web: Control Page =====================
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32-CAM Car</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 20px; background: #f0f0f0; }
      img#stream { width: 90%; max-width: 480px; border: 4px solid #333; border-radius: 8px; }
      .btn { display: inline-block; width: 120px; padding: 20px 0; margin: 10px;
             background: #3498db; color: white; font-size: 24px; border: none;
             border-radius: 50px; cursor: pointer; box-shadow: 0 4px #999; }
      .btn:active { transform: translateY(4px); box-shadow: 0 1px #666; }
      .btn-stop { background: #e74c3c; }
      .row { margin: 15px 0; }
      #status { margin-top: 20px; font-size: 18px; background: #333; color: #0f0;
                padding: 10px; border-radius: 8px; display: inline-block; min-width: 200px; }
      .debug { margin-top: 30px; border-top: 1px solid #ccc; padding-top: 20px; }
      .debug .btn { width: 80px; padding: 10px 0; font-size: 16px; background: #95a5a6; }
    </style>
  </head>
  <body>
    <h1>&#128663; ESP32-CAM Car</h1>
    <img id="stream" src="/stream">
    <div class="row"><button class="btn" onclick="sendCmd('forward')">&#9650; Forward</button></div>
    <div class="row">
      <button class="btn" onclick="sendCmd('left')">&#9668; Left</button>
      <button class="btn btn-stop" onclick="sendCmd('stop')">&#9632; Stop</button>
      <button class="btn" onclick="sendCmd('right')">&#9658; Right</button>
    </div>
    <div class="row"><button class="btn" onclick="sendCmd('backward')">&#9660; Backward</button></div>
    <div id="status">Ready</div>

    <div class="debug">
      <h3>Debug - test each motor</h3>
      <button class="btn" onclick="sendCmd('test?motor=0&dir=fwd')">L-Fwd</button>
      <button class="btn" onclick="sendCmd('test?motor=0&dir=bwd')">L-Bwd</button>
      <button class="btn" onclick="sendCmd('test?motor=1&dir=fwd')">R-Fwd</button>
      <button class="btn" onclick="sendCmd('test?motor=1&dir=bwd')">R-Bwd</button>
      <button class="btn btn-stop" onclick="sendCmd('stop')">Stop</button>
    </div>

    <script>
      function sendCmd(cmd) {
        const status = document.getElementById('status');
        status.innerText = 'Sending...';
        fetch('/' + cmd)
          .then(response => response.text())
          .then(data => { status.innerText = data; })
          .catch(err => {
            status.innerText = 'Error: ' + err;
            setTimeout(() => {
              fetch('/' + cmd)
                .then(res => res.text())
                .then(data => status.innerText = data)
                .catch(e => status.innerText = 'Retry failed: ' + e);
            }, 1000);
          });
      }
    </script>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// ===================== Web: Motor Handlers =====================
void handleForward()  { moveForward();  server.send(200, "text/plain", "Forward"); }
void handleBackward() { moveBackward(); server.send(200, "text/plain", "Backward"); }
void handleLeft()     { turnLeft();     server.send(200, "text/plain", "Left"); }
void handleRight()    { turnRight();    server.send(200, "text/plain", "Right"); }
void handleStop()     { stopMotors();   server.send(200, "text/plain", "Stopped"); }

void handleTest() {
  if (server.hasArg("motor") && server.hasArg("dir")) {
    int motor = server.arg("motor").toInt();
    bool fwd = server.arg("dir") == "fwd";
    testMotor(motor, fwd);
    server.send(200, "text/plain", String("Test motor ") + motor + (fwd ? " forward" : " backward"));
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// ===================== Web: MJPEG Stream Handler =====================
// Lightweight multipart stream served on the same WebServer/port as
// everything else, so no second port or app_httpd.cpp is needed.
void handleStream() {
  WiFiClient client = server.client();

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    server.sendContent("--frame\r\n");
    server.sendContent("Content-Type: image/jpeg\r\n\r\n");
    client.write(fb->buf, fb->len);
    server.sendContent("\r\n");

    esp_camera_fb_return(fb);

    if (!client.connected()) break;
  }
}

// Optional: single still-image capture endpoint
void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ===================== Setup =====================
void setup() {
  // ---- 1. Motor pins: stop immediately before anything else ----
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Starting ESP32-CAM Car Control");

  // ---- 2. Camera init (mirrors the official example's logic) ----
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;   // bump up if you want, PSRAM has room
    config.jpeg_quality  = 10;
    config.fb_count      = 2;
    config.fb_location   = CAMERA_FB_IN_PSRAM;
    config.grab_mode     = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size   = FRAMESIZE_QVGA;  // keep it small without PSRAM
    config.jpeg_quality  = 12;
    config.fb_count      = 1;
    config.fb_location   = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  } else {
    Serial.println("Camera OK");
    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);
      s->set_brightness(s, 1);
      s->set_saturation(s, -2);
    }
  }

  // ---- 3. WiFi ----
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. Open http://");
  Serial.println(WiFi.localIP());

  // ---- 4. Web server routes ----
  server.on("/", handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/test", handleTest);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
