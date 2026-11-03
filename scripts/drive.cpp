/*
 * ESP32-CAM Car Control — ENA+ENB bridged on GPIO12, stream on core 0
 * ------------------------------------------------------------------
 *  ARCHITECTURE:
 *    • WebServer(80)         → all control endpoints, runs in loop() on core 1
 *    • WiFiServer(81)        → MJPEG stream, runs in a FreeRTOS task on core 0
 *    • Therefore button requests never queue behind the video stream
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <nvs_flash.h>

// ===================== WiFi =====================
const char *ssid     = "wifi-name";
const char *password = "wifi-password";

// ===================== Motor pins =====================
#define IN1  2    // left  +
#define IN2  14   // left  -
#define IN3  15   // right +
#define IN4  13   // right -
#define MOTOR_EN 12   // bridged ENA+ENB (both L298N channels enabled together)

// ===================== Flash LED =====================
#define LED_PIN      4
#define PWM_FREQ_LED 5000
#define PWM_RES_LED  8
int ledLevel = 0;

// ===================== Camera pins (AI-Thinker) =====================
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

// ===================== Servers =====================
WebServer server(80);   // control endpoints, runs in loop() on core 1

// ===================== Motor functions =====================
void moveForward()  { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW );
                      digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW ); }
void moveBackward() { digitalWrite(IN1,LOW ); digitalWrite(IN2,HIGH);
                      digitalWrite(IN3,LOW ); digitalWrite(IN4,HIGH); }
void turnLeft()     { digitalWrite(IN1,LOW ); digitalWrite(IN2,LOW );
                      digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW ); }
void turnRight()    { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW );
                      digitalWrite(IN3,LOW ); digitalWrite(IN4,LOW ); }
void spinLeft()     { digitalWrite(IN1,LOW ); digitalWrite(IN2,HIGH);
                      digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW ); }
void spinRight()    { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW );
                      digitalWrite(IN3,LOW ); digitalWrite(IN4,HIGH); }
void stopMotors()   { digitalWrite(IN1,LOW ); digitalWrite(IN2,LOW );
                      digitalWrite(IN3,LOW ); digitalWrite(IN4,LOW ); delay(10); }

void testMotor(int motor, bool forward) {
  if (motor == 0) {
    digitalWrite(IN1, forward ? HIGH : LOW);
    digitalWrite(IN2, forward ? LOW  : HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, forward ? HIGH : LOW);
    digitalWrite(IN4, forward ? LOW  : HIGH);
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  }
}

void setLed(int level) {
  ledLevel = constrain(level, 0, 255);
  ledcWrite(LED_PIN, ledLevel);
}

// ===================== HTML =====================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM Car</title>
<style>
 body{font-family:Arial;text-align:center;margin-top:20px;background:#f0f0f0}
 img#stream{width:90%;max-width:480px;border:4px solid #333;border-radius:8px;background:#000}
 .btn{display:inline-block;width:110px;padding:18px 0;margin:8px;background:#3498db;color:#fff;
      font-size:20px;border:none;border-radius:50px;cursor:pointer;box-shadow:0 4px #999}
 .btn:active{transform:translateY(4px);box-shadow:0 1px #666}
 .btn-stop{background:#e74c3c}.btn-spin{background:#9b59b6}.btn-led{background:#f1c40f;color:#333}
 .row{margin:12px 0}
 #status{margin-top:20px;font-size:18px;background:#333;color:#0f0;padding:10px;
         border-radius:8px;display:inline-block;min-width:200px}
 .debug{margin-top:20px;border-top:1px solid #ccc;padding-top:20px}
 .debug .btn{width:80px;padding:10px 0;font-size:16px;background:#95a5a6}
</style></head><body>
<h1>&#128663; ESP32-CAM Car</h1>
<img id="stream">
<h3>Drive</h3>
<div class="row"><button class="btn" onclick="sendCmd('forward')">&#9650; Forward</button></div>
<div class="row">
 <button class="btn" onclick="sendCmd('left')">&#9668; Turn L</button>
 <button class="btn btn-stop" onclick="sendCmd('stop')">&#9632; Stop</button>
 <button class="btn" onclick="sendCmd('right')">&#9658; Turn R</button>
</div>
<div class="row"><button class="btn" onclick="sendCmd('backward')">&#9660; Backward</button></div>
<h3>Spin</h3>
<div class="row">
 <button class="btn btn-spin" onclick="sendCmd('spinleft')">&#8630; Spin L</button>
 <button class="btn btn-spin" onclick="sendCmd('spinright')">&#8631; Spin R</button>
</div>
<h3>Light</h3>
<div class="row">
 <button class="btn btn-led" onclick="setLed(255)">&#128161; On</button>
 <button class="btn btn-led" onclick="setLed(0)">Off</button>
</div>
<div>
 <label>Brightness: <span id="ledVal">0</span></label><br>
 <input type="range" id="ledSlider" min="0" max="255" value="0"
        oninput="document.getElementById('ledVal').innerText=this.value"
        onchange="setLed(this.value)">
</div>
<div id="status">Ready</div>
<div class="debug">
 <h3>Debug – test each motor</h3>
 <button class="btn" onclick="sendCmd('test?motor=0&dir=fwd')">L-Fwd</button>
 <button class="btn" onclick="sendCmd('test?motor=0&dir=bwd')">L-Bwd</button>
 <button class="btn" onclick="sendCmd('test?motor=1&dir=fwd')">R-Fwd</button>
 <button class="btn" onclick="sendCmd('test?motor=1&dir=bwd')">R-Bwd</button>
 <button class="btn btn-stop" onclick="sendCmd('stop')">Stop</button>
</div>
<script>
 // Stream runs on port 81 (separate core) — never blocks these buttons
 document.getElementById('stream').src = 'http://' + location.hostname + ':81/';
 function sendCmd(cmd){
   const s = document.getElementById('status');
   s.innerText = 'Sending...';
   fetch('/' + cmd).then(r => r.text()).then(t => s.innerText = t)
     .catch(e => s.innerText = 'Error: ' + e);
 }
 function setLed(v){
   document.getElementById('ledVal').innerText = v;
   document.getElementById('ledSlider').value = v;
   fetch('/led?value=' + v).then(r => r.text())
     .then(t => document.getElementById('status').innerText = t);
 }
</script>
</body></html>
)rawliteral";
  server.send(200, "text/html", html);
}

// ===================== Control endpoint handlers =====================
void handleForward()  { moveForward();  server.send(200,"text/plain","Forward"); }
void handleBackward() { moveBackward(); server.send(200,"text/plain","Backward"); }
void handleLeft()     { turnLeft();     server.send(200,"text/plain","Turn Left"); }
void handleRight()    { turnRight();    server.send(200,"text/plain","Turn Right"); }
void handleSpinLeft() { spinLeft();     server.send(200,"text/plain","Spin Left"); }
void handleSpinRight(){ spinRight();    server.send(200,"text/plain","Spin Right"); }
void handleStop()     { stopMotors();   server.send(200,"text/plain","Stopped"); }

void handleTest() {
  if (server.hasArg("motor") && server.hasArg("dir")) {
    int  m   = server.arg("motor").toInt();
    bool fwd = server.arg("dir") == "fwd";
    testMotor(m, fwd);
    server.send(200,"text/plain", String("Test m") + m + (fwd?" fwd":" bwd"));
  } else server.send(400,"text/plain","Missing params");
}
void handleLedOn()  { setLed(255); server.send(200,"text/plain","LED On"); }
void handleLedOff() { setLed(0);   server.send(200,"text/plain","LED Off"); }
void handleLedLevel() {
  if (server.hasArg("value")) {
    setLed(server.arg("value").toInt());
    server.send(200,"text/plain", String("LED ") + ledLevel);
  } else server.send(400,"text/plain","Missing value");
}

// ===================== MJPEG stream task (runs on core 0) =====================
void streamTask(void *param) {
  WiFiServer streamServer(81);
  streamServer.begin();
  Serial.println("Stream task started on port 81 (core 0)");

  char hdr[96];   // reusable header buffer — avoids String churn per frame

  for (;;) {
    WiFiClient client = streamServer.available();
    if (!client) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    client.setNoDelay(true);
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n");
    client.print("Cache-Control: no-cache\r\n");
    client.print("Connection: close\r\n\r\n");

    Serial.println("Stream client connected");

    while (client.connected()) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) { vTaskDelay(1); continue; }

      // Single buffer build → 3 writes per frame, no String allocs
      int hlen = snprintf(hdr, sizeof(hdr),
        "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
        (unsigned)fb->len);
      client.write((const uint8_t*)hdr, hlen);
      client.write(fb->buf, fb->len);
      client.write((const uint8_t*)"\r\n", 2);
      esp_camera_fb_return(fb);

      // Yield to the WiFi stack; keeps latency low
      vTaskDelay(1);
    }

    client.stop();
    Serial.println("Stream client disconnected");
  }
}

// ===================== Setup =====================
void setup() {
  // ---- 1. Motor direction pins: LOW immediately ----
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);

  // ---- 2. Shared ENA+ENB pin (bridged on the L298N) ----
  pinMode(MOTOR_EN, OUTPUT);
  digitalWrite(MOTOR_EN, HIGH);   // enable both H-bridges

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Starting ESP32-CAM Car Control");

  // ---- 3. NVS self-heal (in case previous corruption) ----
  esp_err_t nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("NVS corrupt — erasing and reinitialising…");
    nvs_flash_erase();
    nvs_flash_init();
  }

  // ---- 4. Camera ----
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
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
  } else {
    Serial.println("Camera OK");
    sensor_t *s = esp_camera_sensor_get();
    if (s && s->id.PID == OV3660_PID) {
      s->set_vflip(s,1); s->set_brightness(s,1); s->set_saturation(s,-2);
    }
  }

  // ---- 5. LED PWM (after camera, so LEDC channel 0 is safe) ----
  ledcAttach(LED_PIN, PWM_FREQ_LED, PWM_RES_LED);
  ledcWrite(LED_PIN, 0);

  // ---- 6. WiFi ----
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("WiFi connected. Open http://");
  Serial.println(WiFi.localIP());

  // ---- 7. Control server on port 80 (runs in loop(), core 1) ----
  server.on("/",            handleRoot);
  server.on("/forward",     handleForward);
  server.on("/backward",    handleBackward);
  server.on("/left",        handleLeft);
  server.on("/right",       handleRight);
  server.on("/spinleft",    handleSpinLeft);
  server.on("/spinright",   handleSpinRight);
  server.on("/stop",        handleStop);
  server.on("/test",        handleTest);
  server.on("/led/on",      handleLedOn);
  server.on("/led/off",     handleLedOff);
  server.on("/led",         handleLedLevel);
  server.begin();
  Serial.println("Control server on port 80 (core 1)");

  // ---- 8. Stream task on core 0 ----
  xTaskCreatePinnedToCore(
    streamTask,          // task function
    "mjpeg_stream",      // task name
    4096,                // stack size (bytes)
    NULL,                // parameter
    1,                   // priority (WiFi uses 23; low is fine)
    NULL,                // task handle (unused)
    0                    // pin to core 0 ("the second core")
  );
}

void loop() {
  server.handleClient();
  // Nothing else — stream runs in its own FreeRTOS task on core 0
}
