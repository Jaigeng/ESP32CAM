## IoT Device Based AI Face Verification System
This project presents an embedded system utilizing the ESP32-CAM as an image acquisition device. The system is designed to capture facial images, transmit the data wirelessly, and employ MQTT as an intermediary communication protocol between the edge device and the backend server. The backend performs face verification.

## System Architecture

The proposed system adopts a **client–broker–server architecture**, consisting of the following components:

- **ESP32-CAM (Edge Device)**  
  Responsible for image capture and data transmission.

- **MQTT Broker**  
  Acts as a lightweight message relay between the edge device and the backend server.

- **Backend Server**  
  Performs face verification and generates de-identified results.

---

## Tools Used

- **Hardware**: ESP32-CAM  
- **Communication Protocol**: MQTT over Wi-Fi  
- **Backend Processing**: Face verification algorithm (CNN)  
- **Commmunication**: MQTT Broker
