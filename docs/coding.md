
# Coding
For the IDE (integrated development environment), choose Arduino IDE or VScode + PlatformIO Plugin

Arduino IDE ESP32-CAM libraries take a long time to download

## Setup (VS code + Platform IO)
1. [VS code](https://code.visualstudio.com/download?_exp_download=fb315fc982)
2. search and install PlatformIO plugin
3. Create a project
4. Choose device as **AI thinker ESP32 Cam**

[setup.webm](https://github.com/user-attachments/assets/9860006f-be23-491a-b1ea-f8f2e525caa0)


### ESP32 Cam Blink Test
1. go to **(project)>src > main.cpp**
2. copy and paste [blink test](https://github.com/nturobotics/rover/blob/main/scripts/blink_test.cpp) file
3. build and upload
4. Disconnect cable to stop the test
5. delete previous processes in bottom right panel. (build) (upload) (monitor)
6. press clean (trash bin icon)

### Connect Wifi camera
1. go to **(project) > platformio.ini**
2. delete everything, copy and paste this
```
[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200
monitor_rts = 0
monitor_dtr = 0
```
3. go to **(project) > src > main.cpp**
4. copy and paste [camera](https://github.com/nturobotics/rover/blob/main/scripts/camera.cpp) file
5. build and upload
6. open serial monitor **(plug icon)**


### Test Drive
1. go to **(project) > platformio.ini**
2. delete everything, copy and paste [test_drive.ini]()

## Arduino IDE
1. [Download](https://www.arduino.cc/en/software/)
2. [Guide](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/)
3. [Finding our board](https://www.youtube.com/watch?v=z67mfL63e2M)

### Wireless Test
1. copy and paste [wireless test](wireless_test.cpp) file
2. change wifi SSID and password
3. upload (top left **right arrow** symbol)
4. open serial monitor (top right **magnifying glass** symbol)
5. choose **115200** baud (bottom right dropdown menu)
6. copy wifi **{IP Address}**
7. paste server link in browser **http://{IP Address}/L**
8. Press turn on and off light on server page

### Camera
1. [guide](https://lastminuteengineers.com/getting-started-with-esp32-cam/)

