# Circuit
<img width="600" alt="image" src="https://github.com/user-attachments/assets/ae107493-a26b-4a37-8dc5-e43c11870b44" />

[Circuit Design](https://app.cirkitdesigner.com/project/c6ccce3c-a848-4c31-a3ae-477986d8f76b)

## L298N Driver board internal wiring
<img width="396" alt="Screenshot from 2026-10-14 16-13-36" src="https://github.com/user-attachments/assets/b0799218-9c79-46ab-8366-6e64080c76bf" />

Positive - This allows us to use less GPIO pins on the ESP32-CAM.
Negative - We cannot control the speed of the motors. Only switching them on and off 

## Power connections

<img width="580" alt="Screenshot from 2026-10-14 16-18-40" src="https://github.com/user-attachments/assets/afac9c48-324a-4bcb-9054-66ea6dabb49d" />

We use the 5V output of the Driver Board to power the ESP32-CAM

Positive - no separate voltage converter needed
Negative - Power supply must be stable and high enough or else WIFI signal on ESP32-CAM will not work. Battery must be charged to provide enough current and voltage 

## Control Pin connections
<img width="447" alt="Screenshot from 2026-10-14 16-40-40" src="https://github.com/user-attachments/assets/bb97225b-5926-40ee-8691-b56b4f08ed68" />

## Motor connections
<img width="440" alt="Screenshot from 2026-10-14 16-58-02" src="https://github.com/user-attachments/assets/d0d2626f-5233-491c-b76d-fd591cf5c77d" />
<img width="336" alt="Screenshot from 2026-10-14 17-03-19" src="https://github.com/user-attachments/assets/2a7cd4db-dd03-4c9c-86e7-1e9896e0e551" />

We don't know which way the motor will spin yet, so it doesn't matter which wire goes into which hole. Later after uploading the code we will know. Then we switch the wiring.

## ESP32-CAM USB connection
<img width="663" alt="Screenshot from 2026-10-14 17-28-20" src="https://github.com/user-attachments/assets/de5a3ecc-cb86-4529-9b5f-2f09772c865e" />

if your laptop can connect to USB C as well, then you can swap USB A head with a USB C head.



