
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
1. go to **(project) > src > main.cpp**
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
2. delete everything, copy and paste [drive.ini](https://github.com/nturobotics/rover/blob/main/scripts/drive.ini)
4. go to **(project) > src > main.cpp**
5. copy and paste [https://github.com/nturobotics/rover/blob/main/scripts/drive.cpp) file
6. change wifi and password to the wifi you want to use

<img width="400" alt="image" src="https://github.com/user-attachments/assets/24378f96-c4e3-449f-a7dc-dfa81cdf43a8" />

8. connect your laptop and any device you want to use to control the **Rover** to that wifi
9. press **File > Save All**
10. upload
11. open serial monitor **(plug icon)**

<img width="281" alt="image" src="https://github.com/user-attachments/assets/2ae9072a-349d-475f-8d3f-064e4a66e1dc" />

13. copy the server link **http://{wifi_id}** to browser
  
  important: *DO NOT* use **https** (the "s" means *secure* so you cannot open the server link)

<img width="331" alt="image" src="https://github.com/user-attachments/assets/9c32d54a-dff7-40db-b2a0-3ba02ed07cc4" />

14. unplug the ESP32-CAM and connect to **Rover**
15. put batteries in (some wheels will autorun for a bit, it's normal)
16. control the **Rover** with your phone or laptop

<img width="496" alt="image" src="https://github.com/user-attachments/assets/3a29af3e-7f58-4799-be5e-7591472c2c6f" />

## Arduino IDE
[Documentation](https://github.com/nturobotics/rover/blob/main/docs/arduino_ide.md)
