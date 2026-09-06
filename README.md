# Rover

## Setup
### Install Arduino IDE
1. [Download](https://www.arduino.cc/en/software/)
2. [Guide](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/)
3. [Finding our board](https://www.youtube.com/watch?v=z67mfL63e2M)
4. 

### PlatformIO + VS code plugin
1. [VS code](https://code.visualstudio.com/download?_exp_download=fb315fc982)
2. search and install PlatformIO plugin
3. Create a project
4. Choose device as **AIthinker ESP32 Cam**

#### ESP32 Cam Blink Test
1. go to **(project)>src > main.ccp**
2. copy and paste [blink test](blink_test.cpp) file

3. build and upload
4. Disconnect cable to stop the test
5. delete previous processes in bottom right panel. (build) (upload) (monitor)
6. press clean (trash bin icon)


#### Connect Wifi
1. go to **(project) > platformio.ini**
2. copy and paste under everything else
```
monitor_speed = 115200 // serial monitor will show garbled text *?xx??xxxxxx* if incorrect speed
monitor_rts = 0
monitor_dtr = 0
```
3. go to **(project)>src > main.ccp**
4. copy and paste [wifi scan](wifi_scan.cpp) file
5. build and upload
6. open serial monitor **(plug icon)**
7. network names and types will appear

   if garbled text appears, incorrect or mismatched speed.

## Guides
- [ESP starter](https://deepbluembedded.com/getting-started-with-esp32-programming-tutorials/)
