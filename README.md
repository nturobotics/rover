# Table of Contents
1. [Bill of Materials](#bom)
2. [ESP32-Cam Setup](#esp32-cam)
3. [3D Design](#3d-design)
   
   3.1 [Container](#container)

   3.2 [Lid](#lid)

6. [Circuit](#circuit)
7. [ESP Guide](#esp-guide)

# BOM
[v1.0](https://docs.google.com/spreadsheets/d/1K54std629R6cTmjSbiGCCC1N0AOHtXluDwHZW1lQWgc/edit?usp=sharing)

## L298N Motor Driver
![L298N Motor Driver](https://arduinoyard.com/wp-content/uploads/2025/02/l298n_motordriver_pinout_bb.png)
  
## Q054 geared motor TT 130
![TT Motor](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/_images/motor_size.jpg)
  
## Q054 geared motor TT 130 Bracket
![TT Motor Bracket](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Ftse1.mm.bing.net%2Fth%2Fid%2FOIP.OWw1BkBCed7D1dgC306kpQHaIO%3Fpid%3DApi&f=1&ipt=8cd32029b7c4f3217929b2442db31fc445e7eeda349d952212136bd15dc7ec54)

## Q016 rubber tire (20T65) 65MM
![Q016 rubber tire (20T65) 65MM](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fsc04.alicdn.com%2Fkf%2FHca8820e923f0480a8a8a955f87e4b8dew%2F231573752%2FHca8820e923f0480a8a8a955f87e4b8dew.jpg&f=1&nofb=1&ipt=85a892d917c41d6dc53f03f6b90fd0caf973b449a729b962d31e61467ef7a168)

## XL6009 DC-DC Voltage Booster
![XL6009 DC-DC Voltage Booster](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fblogger.googleusercontent.com%2Fimg%2Fb%2FR29vZ2xl%2FAVvXsEgIvwAsF3_N7-x4d9W4hWRnFzd_pLYQCGDoJ2oeoHnM_UZIGvTMCecxUccxBcEN-Ktrwi12Wf2KNKnET0z_ZULt-gCL6Wvzg2OqtgEcVLp0j9kMXYWI8pCIwksrZeClLxWXakHLFZEjE2Ou3vgmc_QI-MCytz6umX5aZrrgxC3oq7OxFrzZ2N5fRm-DW1N4%2Fs1280%2FSchematic-of-xl6019-adjustable-dc-to-dc-step-up-converter-circuit_Electrothinks.png&f=1&nofb=1&ipt=ca9245dfe2e435692e0b96a8f5613912d5cf912885c10ed7d569d3f5dc712c27)

## LM2596 DC-DC Voltage Bucker ( + Display + Screw Terminal Block )
![LM2596 DC-DC Voltage Bucker](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Feleberric.com%2Fwp-content%2Fuploads%2F2025%2F06%2FLM2596-Buck-Converter-with-Display.jpg&f=1&nofb=1&ipt=03f64bfda6245f896e6668fac094b70c8072c5945c338ef890a414006372121c)

## 18650 Battery
![18650 battery](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Ftse2.mm.bing.net%2Fth%2Fid%2FOIP.WDeJ6twWpFDkR6nIgntv9QHaHa%3Fr%3D0%26pid%3DApi&f=1&ipt=ab356e589da4fb0a92e15865c9a8f4ddb21aa08ea32c65e3c8fa7a9c927967ae&ipo=images)

# ESP32-Cam
Arduino IDE OR VScode + PlatformIO Plugin
## Install Arduino IDE
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


## PlatformIO + VS code plugin
1. [VS code](https://code.visualstudio.com/download?_exp_download=fb315fc982)
2. search and install PlatformIO plugin
3. Create a project
4. Choose device as **AIthinker ESP32 Cam**

### ESP32 Cam Blink Test
1. go to **(project)>src > main.cpp**
2. copy and paste [blink test](blink_test.cpp) file

3. build and upload
4. Disconnect cable to stop the test
5. delete previous processes in bottom right panel. (build) (upload) (monitor)
6. press clean (trash bin icon)


### Connect Wifi
1. go to **(project) > platformio.ini**
2. copy and paste under everything else
```
monitor_speed = 115200 // serial monitor will show garbled text *?xx??xxxxxx* if incorrect speed
monitor_rts = 0
monitor_dtr = 0
```
3. go to **(project)>src > main.cpp**
4. copy and paste [wifi scan](wifi_scan.cpp) file
5. build and upload
6. open serial monitor **(plug icon)**
7. network names and types will appear

   if garbled text appears, incorrect or mismatched speed.



[OnShape Design](https://cad.onshape.com/documents/e64b89430876cdb7d6b69ab3/w/77c1ce2ce73c963871a615a2/e/e7f76abc5f1c2211f29a23ec?renderMode=0&uiState=6aaf8ad37668e8bcef2a1785)
# 3D-Design
1. Units are all in milimeters (mm)

## Container
![](Images/container.png)
### Outer Wall
[](https://github.com/user-attachments/assets/a5e99203-3ade-498d-809d-e49bd2d916f6)

![](Images/outer_wall.png)
### Inner Wall
[](https://github.com/user-attachments/assets/da4df0dd-af4f-4e59-848b-a63e858b195d)

![](Images/inner_wall.png)
### Top Holes
[](https://github.com/user-attachments/assets/27214523-afbd-4703-bcae-0e40a43c85ab)

![](Images/bolt_holes.png)
### Side holes
[side.webm](https://github.com/user-attachments/assets/36426668-020c-44df-9053-4a59281ac936)

![](Images/side_sketch.png)
### Front hole
[](https://github.com/user-attachments/assets/d10900fe-48e4-41cb-896e-6aa9c3cc8334)

![](Images/front.png)

### Bottom boles
![](Images/bottom_holes.png)


## Lid
![](Images/blank_lid.png)

[](https://github.com/user-attachments/assets/992a540b-3695-44bd-847e-d5f1035c15b0)

### Cover
![](Images/lid_sketch.png)
### Holes
![](Images/bolt_holes.png)
### Decorated Lid
- ![](Images/decorated_lid.png)
  
# Circuit
- ![](Images/circuit.png)
[Circuit Design](https://app.cirkitdesigner.com/project/c6ccce3c-a848-4c31-a3ae-477986d8f76b)

  
# ESP-guide
- [ESP starter](https://deepbluembedded.com/getting-started-with-esp32-programming-tutorials/)
