/*
 * ESP32-CAM Car Control — ENA+ENB bridged on GPIO12, stream on core 0
 * ------------------------------------------------------------------
 *  ARCHITECTURE:
 *    • WebServer(80)         → all control endpoints, runs in loop() on core 1
 *    • WiFiServer(81)        → MJPEG stream, runs in a FreeRTOS task on core 0
 *    • Therefore button requests never queue behind the video stream
 *
 *  ADDED IN THIS REVISION:
 *    • WiFi connect timeout + background reconnect (no longer hangs forever)
 *    • HTTP Basic Auth on every control endpoint + a shared key on the stream
 *    • ArduinoOTA so you can re-flash over WiFi once it's on the car
 *    • Light-themed control page (chilljudge.com style)
 *
 *  BEFORE FLASHING: change http_username / http_password / ota_password below.
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <nvs_flash.h>

// ===================== WiFi =====================
const char *ssid     = "wifi-name";
const char *password = "wifi-password";

const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;  // give up and retry in background after this
const unsigned long WIFI_RETRY_INTERVAL_MS  = 10000;  // how often loop() retries while disconnected
unsigned long wifiRetryAt = 0;

// ===================== Access control =====================
// http_password also doubles as the ?key= on the :81 video stream, which
// can't do real Basic Auth (it's a raw socket, not WebServer).
const char *http_username = "admin";
const char *http_password = "rover1234";       // CHANGE ME
const char *ota_password  = "rover-ota-1234";   // CHANGE ME

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

// ===================== Auth helper =====================
bool requireAuth() {
  if (!server.authenticate(http_username, http_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

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

// ===================== HTML (light theme, chilljudge.com style) =====================
void handleRoot() {
  if (!requireAuth()) return;

  String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rover Control</title>
<style>
 :root{
   --bg:#f1f5f9; --card:#ffffff; --border:#e2e8f0;
   --text:#334155; --muted:#64748b; --faint:#94a3b8;
   --accent:#14b8a6; --accent-light:#2dd4bf; --danger:#f43f5e;
   --radius-card:24px; --radius-inner:16px;
   --shadow:0 4px 20px rgba(0,0,0,0.06);
 }
 *{box-sizing:border-box}
 body{
   margin:0; min-height:100vh; background:var(--bg); color:var(--text);
   font-family:-apple-system,"PingFang TC","Noto Sans TC",sans-serif;
   display:flex; flex-direction:column; align-items:center; gap:14px;
   padding:16px 16px 40px;
 }
 .topbar{
   display:flex; align-items:center; gap:10px; width:100%; max-width:420px;
   background:rgba(255,255,255,0.9); backdrop-filter:blur(6px);
   border:0.8px solid var(--border); border-radius:999px;
   padding:10px 16px; box-shadow:var(--shadow);
 }
 .topbar .logo{font-size:18px}
 .topbar h1{font-size:15px; font-weight:600; margin:0; flex:1}
 .pill{
   display:inline-flex; align-items:center; gap:6px;
   font-size:12px; color:var(--muted);
   background:#f8fafc; border:0.8px solid var(--border);
   border-radius:999px; padding:4px 10px;
 }
 .dot{width:7px; height:7px; border-radius:50%; background:var(--faint)}
 .dot.ok{background:var(--accent)}
 .dot.err{background:var(--danger)}
 .card{
   width:100%; max-width:420px; background:var(--card);
   border:0.8px solid var(--border); border-radius:var(--radius-card);
   box-shadow:var(--shadow); padding:18px;
 }
 .stream-wrap{
   width:100%; aspect-ratio:4/3; background:#0f172a;
   border-radius:var(--radius-inner); border:0.8px solid var(--border);
   overflow:hidden; display:flex; align-items:center; justify-content:center;
   margin-bottom:16px;
 }
 .stream-wrap img{width:100%; height:100%; object-fit:cover; display:block}
 h3.section{
   font-size:11px; font-weight:600; letter-spacing:.08em; text-transform:uppercase;
   color:var(--faint); margin:18px 0 10px;
 }
 h3.section:first-of-type{margin-top:0}
 .btn{
   appearance:none; border:0.8px solid var(--border); background:#ffffff; color:var(--text);
   font-size:15px; font-weight:500; border-radius:999px; padding:14px 0;
   cursor:pointer; box-shadow:0 1px 0 rgba(0,0,0,0.03);
   transition:border-color .15s, transform .05s;
 }
 .btn:active{transform:translateY(1px)}
 .btn:hover{border-color:#cbd5e1}
 .btn-stop{background:var(--danger); color:#fff; border:none}
 .btn-accent{background:var(--accent); color:#fff; border:none}
 .btn-accent:hover{background:var(--accent-light)}
 .dpad{display:grid; grid-template-columns:1fr 1fr 1fr; gap:8px}
 .dpad .f{grid-column:2}
 .dpad .l{grid-column:1}
 .dpad .s{grid-column:2}
 .dpad .r{grid-column:3}
 .dpad .b{grid-column:2}
 .row{display:flex; gap:8px; margin-bottom:8px}
 .row .btn{flex:1}
 .light-row{display:flex; gap:8px; margin-bottom:10px}
 .light-row .btn{flex:1}
 input[type=range]{width:100%; accent-color:var(--accent)}
 .brightness-label{font-size:13px; color:var(--muted); margin-bottom:6px; display:flex; justify-content:space-between}
 #status{
   margin-top:16px; font-size:13px; color:var(--muted); background:#f8fafc;
   border:0.8px solid var(--border); border-radius:var(--radius-inner);
   padding:10px 12px; text-align:center;
 }
 .debug{margin-top:18px; border-top:0.8px solid var(--border); padding-top:14px}
 .debug .row .btn{font-size:13px; padding:10px 0; color:var(--muted)}
</style></head><body>

<div class="topbar">
  <span class="logo">&#128663;</span>
  <h1>Rover Control</h1>
  <span class="pill"><span class="dot ok" id="statusDot"></span><span id="statusText">ready</span></span>
</div>

<div class="card">
  <div class="stream-wrap"><img id="stream" alt="live stream"></div>

  <h3 class="section">Drive</h3>
  <div class="dpad">
    <button class="btn f" onclick="sendCmd('forward')">&#9650;</button>
    <button class="btn l" onclick="sendCmd('left')">&#9668;</button>
    <button class="btn btn-stop s" onclick="sendCmd('stop')">&#9632;</button>
    <button class="btn r" onclick="sendCmd('right')">&#9658;</button>
    <button class="btn b" onclick="sendCmd('backward')">&#9660;</button>
  </div>

  <h3 class="section">Spin</h3>
  <div class="row">
    <button class="btn btn-accent" onclick="sendCmd('spinleft')">&#8630; Spin L</button>
    <button class="btn btn-accent" onclick="sendCmd('spinright')">&#8631; Spin R</button>
  </div>

  <h3 class="section">Light</h3>
  <div class="light-row">
    <button class="btn btn-accent" onclick="setLed(255)">&#128161; On</button>
    <button class="btn" onclick="setLed(0)">Off</button>
  </div>
  <div class="brightness-label"><span>Brightness</span><span id="ledVal">0</span></div>
  <input type="range" id="ledSlider" min="0" max="255" value="0"
         oninput="document.getElementById('ledVal').innerText=this.value"
         onchange="setLed(this.value)">

  <div id="status">Ready</div>

  <div class="debug">
    <h3 class="section">Debug — per-motor test</h3>
    <div class="row">
      <button class="btn" onclick="sendCmd('test?motor=0&dir=fwd')">L-Fwd</button>
      <button class="btn" onclick="sendCmd('test?motor=0&dir=bwd')">L-Bwd</button>
    </div>
    <div class="row">
      <button class="btn" onclick="sendCmd('test?motor=1&dir=fwd')">R-Fwd</button>
      <button class="btn" onclick="sendCmd('test?motor=1&dir=bwd')">R-Bwd</button>
    </div>
    <div class="row"><button class="btn btn-stop" onclick="sendCmd('stop')">Stop</button></div>
  </div>
</div>

<script>
 // Stream runs on its own port (core 0) and carries the shared key as a
 // query param, since the raw stream socket can't do Basic Auth.
 document.getElementById('stream').src = 'http://' + location.hostname + ':81/?key=STREAM_KEY_PLACEHOLDER';

 function setStatus(text, state){
   document.getElementById('statusText').innerText = text;
   document.getElementById('statusDot').className = 'dot ' + (state || '');
 }
 function sendCmd(cmd){
   const s = document.getElementById('status');
   s.innerText = 'Sending...';
   fetch('/' + cmd).then(r => r.text()).then(t => { s.innerText = t; setStatus('connected','ok'); })
     .catch(e => { s.innerText = 'Error: ' + e; setStatus('offline','err'); });
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

  html.replace("STREAM_KEY_PLACEHOLDER", http_password);
  server.send(200, "text/html", html);
}

// ===================== Control endpoint handlers =====================
void handleForward()  { if (!requireAuth()) return; moveForward();  server.send(200,"text/plain","Forward"); }
void handleBackward() { if (!requireAuth()) return; moveBackward(); server.send(200,"text/plain","Backward"); }
void handleLeft()     { if (!requireAuth()) return; turnLeft();     server.send(200,"text/plain","Turn Left"); }
void handleRight()    { if (!requireAuth()) return; turnRight();    server.send(200,"text/plain","Turn Right"); }
void handleSpinLeft() { if (!requireAuth()) return; spinLeft();     server.send(200,"text/plain","Spin Left"); }
void handleSpinRight(){ if (!requireAuth()) return; spinRight();    server.send(200,"text/plain","Spin Right"); }
void handleStop()     { if (!requireAuth()) return; stopMotors();   server.send(200,"text/plain","Stopped"); }

void handleTest() {
  if (!requireAuth()) return;
  if (server.hasArg("motor") && server.hasArg("dir")) {
    int  m   = server.arg("motor").toInt();
    bool fwd = server.arg("dir") == "fwd";
    testMotor(m, fwd);
    server.send(200,"text/plain", String("Test m") + m + (fwd?" fwd":" bwd"));
  } else server.send(400,"text/plain","Missing params");
}
void handleLedOn()  { if (!requireAuth()) return; setLed(255); server.send(200,"text/plain","LED On"); }
void handleLedOff() { if (!requireAuth()) return; setLed(0);   server.send(200,"text/plain","LED Off"); }
void handleLedLevel() {
  if (!requireAuth()) return;
  if (server.hasArg("value")) {
    setLed(server.arg("value").toInt());
    server.send(200,"text/plain", String("LED ") + ledLevel);
  } else server.send(400,"text/plain","Missing value");
}

// ===================== MJPEG stream task (runs on core 0) =====================
// Raw WiFiServer, so auth here is a shared "?key=" query param (checked on
// the request line) rather than real Basic Auth.
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

    client.setTimeout(2000);
    String reqLine = client.readStringUntil('\n');
    bool authorized = reqLine.indexOf(String("key=") + http_password) != -1;

    // Drain the rest of the headers up to the blank line, bounded so a
    // misbehaving client can't hang this task forever.
    for (int i = 0; i < 40; i++) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) break;
    }

    if (!authorized) {
      client.print("HTTP/1.1 401 Unauthorized\r\nConnection: close\r\n\r\n");
      client.stop();
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

// ===================== WiFi =====================
bool connectWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. Open http://");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("WiFi connect timed out — will keep retrying in the background");
  return false;
}

// ===================== OTA =====================
void setupOTA() {
  ArduinoOTA.setHostname("esp32cam-rover");
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() { Serial.println("OTA: starting update"); });
  ArduinoOTA.onEnd([]()   { Serial.println("OTA: update finished"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) Serial.printf("OTA: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready (hostname: esp32cam-rover.local)");
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

  // ---- 6. WiFi (timeout instead of blocking forever; loop() retries) ----
  connectWiFi();

  // ---- 7. OTA (set up even if WiFi isn't connected yet; it just listens) ----
  setupOTA();

  // ---- 8. Control server on port 80 (runs in loop(), core 1) ----
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

  // ---- 9. Stream task on core 0 ----
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
  if (WiFi.status() != WL_CONNECTED && millis() - wifiRetryAt > WIFI_RETRY_INTERVAL_MS) {
    wifiRetryAt = millis();
    Serial.println("WiFi disconnected — reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
  ArduinoOTA.handle();
  server.handleClient();
  // Camera stream runs in its own FreeRTOS task on core 0
}
