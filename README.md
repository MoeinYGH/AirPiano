# AirPiano

Folder layout
- project/   Wokwi + PlatformIO project (Arduino Uno). OPEN THIS FOLDER in VS Code.
- bridge.js  sends Wokwi serial output to the web page
- index.html the piano page (double-click it for Demo mode)

Run it
1. VS Code with the extensions "PlatformIO IDE" and "Wokwi Simulator"; Node.js installed.
2. First time only: F1 -> "Wokwi: Request a New License".
3. File -> Open Folder -> choose the `project` folder.
4. PlatformIO ant icon -> Project Tasks -> uno -> General -> Build (wait for SUCCESS).
5. F1 -> "Wokwi: Start Simulator".
6. In a terminal in THIS folder (next to bridge.js):  node bridge.js
7. Open http://localhost:8787 and click "Wokwi bridge".

Change the HC-SR04 distance in the simulation: click the sensor and drag its distance slider.
project/diagram.json is a ready-made circuit; if you already have your own from wokwi.com, keep yours.
