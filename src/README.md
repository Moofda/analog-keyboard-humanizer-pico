# Analog Keyboard Humanizer — Pico Firmware

Hardware adapter firmware for the Waveshare RP2350-USB-A.
Reads your analog keyboard at full native polling rate and outputs
as a real USB XInput controller to your PC.

## Hardware

```
Analog Keyboard (USB-A) ──► Waveshare RP2350-USB-A (USB-A host port)
                                         │
                                         ▼ (USB-C device port)
                                        PC
```

## How to flash

1. Hold the BOOT button on the RP2350-USB-A while plugging it into your PC
2. It will appear as a USB drive called RPI-RP2
3. Download `analog_keyboard_humanizer.uf2` from the latest GitHub Actions run:
   - Go to the **Actions** tab in this repository
   - Click the latest successful build
   - Download the `analog-keyboard-humanizer` artifact
   - Extract the zip to get the `.uf2` file
4. Drag and drop the `.uf2` file onto the RPI-RP2 drive
5. The Pico will reboot automatically and start running

## How to use

1. Flash the firmware (see above)
2. Plug your analog keyboard into the USB-A port on the RP2350
3. Plug the RP2350 USB-C port into your PC
4. Windows will detect it as an Xbox 360 Controller
5. Select it as Player 1 in your game (the keyboard's direct connection is no longer needed)

## Building from source (GitHub Actions — no tools needed)

1. Fork this repository on GitHub
2. Push any change to the `main` branch
3. GitHub Actions will automatically compile and produce a `.uf2`
4. Download from the Actions tab → latest run → Artifacts

## Config tool

A Windows GUI config tool is included in the `config-tool` folder.
Connect the RP2350 via USB-C, run the config tool, and adjust settings live.
Settings are saved to the RP2350's flash memory automatically.

## Settings

All settings are adjustable via the config tool:

| Setting | Description |
|---|---|
| Magnitude Cap | Prevents perfect 1.0 output, keeps output inside circle |
| Magnitude Variation | Subtle wobble on magnitude while held |
| Global Baseline | Overall humanization strength |
| Drift Speed | How quickly position drifts to new target |
| Hold Time Min/Max | How long it stays near one position |
| Deadzone Threshold | Below this, no humanization applied |
| Release Fade | How quickly offset fades on key release |
| Humanization Curve | Shape humanization across deflection range |

## Firmware version

v1.0.0
