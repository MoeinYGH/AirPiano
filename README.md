# AirPiano

A touch-free piano for the Arduino Uno. Move your hand over an HC-SR04 ultrasonic sensor to play the C major scale on a buzzer, watch an RGB LED change color with every note, then record a short song and play it back with the same note lengths and colors.

The project is built for the Wokwi simulator and was written for the Microprocessors Lab course (Assignment 3, Shiraz University, Fall 2026). It also includes an optional web piano that mirrors what the simulated circuit is doing in real time.

## Table of contents

- [Features](#features)
- [Hardware and pin map](#hardware-and-pin-map)
- [Notes and colors](#notes-and-colors)
- [How it works](#how-it-works)
- [Repository layout](#repository-layout)
- [Running the project](#running-the-project)
  - [Option A: Wokwi in the browser](#option-a-wokwi-in-the-browser-quickest)
  - [Option B: Wokwi in VS Code with PlatformIO](#option-b-wokwi-in-vs-code-with-platformio)
  - [Option C: Web piano with the Node.js bridge](#option-c-web-piano-with-the-nodejs-bridge)
- [Using the piano](#using-the-piano)
- [Serial output format](#serial-output-format)
- [Design notes](#design-notes)
- [Limitations](#limitations)
- [Troubleshooting](#troubleshooting)

## Features

- Distance measurement with an HC-SR04, computed from the ECHO pulse width using `pulseIn()`.
- Eight playable notes (C4 to C5), one per 5 cm band between 5 cm and 45 cm. The pitch jumps from note to note and never slides.
- Silence outside the 5 to 45 cm range.
- A unique rainbow color per note, from red (C4) to violet (C5), driven with PWM.
- LED brightness follows the tuning: brightest in the middle of a band, dimmer near its edges.
- Record up to 32 notes with their durations while the REC button is held.
- Playback of the recorded song with the same note lengths and colors, with every note and its length printed to the Serial Monitor (for example `E4 450ms`).
- Optional browser piano that displays the live state of the simulation (keys, distance, brightness) and can play sound.

## Hardware and pin map

Board: Arduino Uno (simulated in Wokwi).

| Part | Wokwi part | Connection |
|---|---|---|
| Distance sensor | HC-SR04 Ultrasonic Distance Sensor | TRIG to D7, ECHO to D6, VCC to 5V, GND to GND |
| Buzzer | Buzzer | D8 to buzzer to GND |
| RGB LED | RGB LED (common cathode) | R to D9, G to D10, B to D5, COM to GND |
| REC button | Pushbutton | D2 to button to GND (`INPUT_PULLUP`) |
| PLAY button | Pushbutton | D4 to button to GND (`INPUT_PULLUP`) |

On real hardware, put a 220 ohm resistor in series with each of the three RGB color pins. The ready-made `project/diagram.json` wires the LED directly, which is fine in the simulator.

## Notes and colors

The playing range is 5 to 45 cm, split into eight bands of 5 cm.

| Note | Distance | Frequency | LED color (R, G, B) |
|---|---|---|---|
| C4 | 5 - 10 cm | 262 Hz | red (255, 0, 0) |
| D4 | 10 - 15 cm | 294 Hz | orange (255, 100, 0) |
| E4 | 15 - 20 cm | 330 Hz | yellow (255, 220, 0) |
| F4 | 20 - 25 cm | 349 Hz | green (0, 255, 0) |
| G4 | 25 - 30 cm | 392 Hz | cyan (0, 220, 220) |
| A4 | 30 - 35 cm | 440 Hz | blue (0, 0, 255) |
| B4 | 35 - 40 cm | 494 Hz | indigo (75, 0, 200) |
| C5 | 40 - 45 cm | 523 Hz | violet (160, 0, 255) |

## How it works

**1. Distance.** The sketch sends a 10 microsecond pulse on TRIG, then measures the width of the pulse on ECHO with `pulseIn()` (30 ms timeout). With `t` in microseconds:

```
d = t * 0.0343 / 2   (cm)
```

If no echo arrives, the reading is treated as "no hand" and the piano stays silent.

**2. Note selection.** For `5 <= d < 45`, the band index is `(d - 5) / 5` (integer part). That index selects both the frequency and the color. Because the frequency is taken from a lookup table, the pitch snaps to exactly one of eight values. Outside the range, `noTone()` is called and the LED is switched off.

**3. Color and brightness.** Each note has an RGB triplet. The position of the hand inside its band (0 to 1) is converted to a "closeness to center" value, and the LED brightness is `0.1 + 0.9 * centre`, so it is 100 percent at the middle of a band and 10 percent at the edges. The triplet is scaled by this brightness and written with `analogWrite()`.

**4. Record and playback.**
- Holding REC starts a new recording (the previous one is discarded). Each time the played note changes, the previous note is stored together with how long it lasted.
- Notes shorter than 40 ms are ignored as glitches.
- Up to 32 notes are stored. Releasing REC saves the song.
- Pressing PLAY (with REC not held and at least one note recorded) plays every stored note on the buzzer for its recorded length, at full brightness in the note's color, and prints each note to the Serial Monitor.

## Repository layout

```
AirPiano/
  README.md        this file
  bridge.js        Node.js bridge: Wokwi serial output -> browser (no dependencies)
  index.html       web piano page (Demo, Wokwi bridge and USB serial modes)
  project/         Wokwi + PlatformIO project (open this folder in VS Code)
    src/main.cpp   the Arduino sketch
    diagram.json   the circuit (Arduino Uno, HC-SR04, buzzer, RGB LED, 2 buttons)
    wokwi.toml     tells the Wokwi VS Code extension where the firmware is
    platformio.ini PlatformIO configuration (board: uno, 9600 baud)
```

The `.pio/` build folder is generated by PlatformIO and is not committed.

## Running the project

### Option A: Wokwi in the browser (quickest)

1. Go to [wokwi.com](https://wokwi.com) and create a new Arduino Uno project.
2. Replace the contents of `diagram.json` with `project/diagram.json` from this repository.
3. Replace the contents of the sketch file (`sketch.ino`) with `project/src/main.cpp`.
4. Click the green Start button.

Note: the browser version of Wokwi cannot pass its serial output to another web page, so the web piano in Option C will not work with it. Use Option B for that.

### Option B: Wokwi in VS Code with PlatformIO

**Requirements**

- [Visual Studio Code](https://code.visualstudio.com/)
- The **PlatformIO IDE** extension
- The **Wokwi Simulator** extension
- [Node.js](https://nodejs.org/) (only needed for Option C)

**Steps**

1. Clone the repository:
   ```
   git clone https://github.com/<your-username>/AirPiano.git
   cd AirPiano
   ```
2. In VS Code, use File > Open Folder and select the **`project`** folder (not the outer `AirPiano` folder). The Wokwi extension looks for `wokwi.toml` in the folder you open.
3. First time only: press `F1`, run **Wokwi: Request a New License**, and follow the browser prompt to activate the free license.
4. Build the firmware: click the PlatformIO ant icon in the sidebar, then Project Tasks > uno > General > Build. Wait for `SUCCESS`. The first build downloads the AVR toolchain, so it can take a few minutes. This creates `.pio/build/uno/firmware.hex` and `firmware.elf`, which `wokwi.toml` points to.
5. Press `F1` and run **Wokwi: Start Simulator**. The circuit opens and the sketch starts running. The Serial Monitor output appears in the simulator panel and begins with `AirPiano ready`.

If you change `main.cpp`, build again before restarting the simulator, because the simulator runs the compiled firmware, not the source file.

### Option C: Web piano with the Node.js bridge

The web piano shows the keys, the measured distance and the LED brightness live. It needs the simulation from Option B to be running, because `bridge.js` connects to the serial server that the Wokwi VS Code extension opens on port 4000 (`rfc2217ServerPort = 4000` in `wokwi.toml`).

1. Start the simulation as described in Option B.
2. Open a terminal in the `AirPiano` folder (the one that contains `bridge.js`) and run:
   ```
   node bridge.js
   ```
   You should see `AirPiano bridge ready -> http://localhost:8787` and then `Wokwi connected`. No `npm install` is needed because the bridge uses only built-in Node modules (Node 14 or newer).
3. Open <http://localhost:8787> in your browser and click **Wokwi bridge**.

Bridge options:

```
node bridge.js                        read Wokwi for VS Code over rfc2217://localhost:4000
node bridge.js --rfc2217 host:port    same, with a custom address
node bridge.js --port 8800            serve the page on a different port
wokwi-cli . | node bridge.js --stdin  read output from the Wokwi CLI instead
```

The page also has two other modes:

- **Demo**: no simulator or Node needed. Just double-click `index.html` and move the cursor over the keys.
- **USB serial**: connects to a real Arduino Uno running this sketch over USB (Chrome or Edge, using the Web Serial API).

## Using the piano

1. Click the HC-SR04 in the running simulation. A distance slider appears. Drag it to move your "hand" between 5 and 45 cm and the buzzer plays the matching note while the LED lights up in the note's color.
2. Move the slider outside the 5 to 45 cm range to silence the piano.
3. **Record:** click and hold the REC button (red), play a short sequence of notes with the slider, then release REC. The Serial Monitor prints `--- Recording ---`, each finished note with its length, and `--- Saved ---`.
4. **Play back:** click the PLAY button (green). The song plays with the same note lengths and colors, and the Serial Monitor prints each note.

In Wokwi, holding a button down while you also drag the slider is easiest if you use the keyboard-free approach: click REC once to start holding it with the mouse, or use the sensor slider with a second mouse action as needed. If this is awkward on your setup, record short phrases note by note and release REC between attempts.

## Serial output format

The serial port runs at 9600 baud.

Messages meant for people:

| Line | Meaning |
|---|---|
| `AirPiano ready` | Printed once at startup |
| `--- Recording ---` | REC was pressed, a new recording started |
| `E4 450ms` | A note and its length, printed when it is recorded and again during playback |
| `--- Saved ---` | REC was released |
| `--- Playback ---` / `--- Done ---` | Start and end of playback |

Telemetry for the web piano (printed whenever the note changes and at least every 100 ms):

```
AP,<note index or -1>,<distance in cm>,<brightness in percent>
```

For example, `AP,2,17.3,86` means note index 2 (E4), 17.3 cm away, LED at 86 percent. A note index of `-1` means silence.

## Design notes

- **Why the blue LED is on D5 and not D11.** `tone()` on the Uno uses Timer2, and that timer also generates the PWM for pins 3 and 11. While a note is playing, `analogWrite()` on D11 would not work properly (the blue channel would be wrong or dead), and the LED color would be incorrect. D5 is driven by Timer0, which is independent of `tone()`, so PWM and sound can run at the same time.
- **ECHO pulse width at 45 cm.** From `d = t * 0.0343 / 2`, `t = 2 * d / 0.0343 = 2 * 45 / 0.0343`, which is about 2624 microseconds.
- **Non-blocking sound.** `tone()` runs on a hardware timer, so the loop keeps measuring distance and updating the LED while a note sounds.

## Limitations

- Pauses (silence) are not recorded. Playback reproduces note lengths, but notes are played back to back with no gaps between them.
- A new recording replaces the previous one.
- The recording buffer holds 32 notes. Notes after the 32nd are ignored.
- During playback the LED is shown at full brightness, not at the brightness it had while recording.
- Live playing is limited by the sensor reading rate (the loop runs about every 30 ms), so very fast note changes can be missed.

## Troubleshooting

| Problem | Fix |
|---|---|
| Wokwi complains about a missing license | Press `F1` and run **Wokwi: Request a New License** |
| Wokwi says the firmware file is missing | Build the project with PlatformIO first (see Option B, step 4) |
| Simulator does not start | Make sure the folder you opened in VS Code is `project`, so that `wokwi.toml` is at its root |
| Bridge prints `Wokwi not running - start the simulation (port 4000)` | Start the simulator. The bridge retries every 2 seconds, so you do not need to restart it |
| Port 8787 is already in use | Run `node bridge.js --port 8800` and open that port instead |
| The web page says "Bridge not reachable" | Run `node bridge.js` and reload the page |
| No sound in the web page | Turn on the sound button on the page (browsers block audio until you interact with the page) |
| Notes never play in the simulation | Check that the HC-SR04 distance slider is between 5 and 45 cm |

## Credits

- Assignment: Microprocessors Lab, Assignment "Air Piano", Shiraz University, Fall 2026.
- Implementation: `<Moein Yeganeh>`

## License

Add a license of your choice (for example MIT) if you want others to reuse the code.
