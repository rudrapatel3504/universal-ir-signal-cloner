# Hardware Wiring and Circuit Details

## Pin Mapping Summary

| Subsystem | ATmega2560 Pin | Connected Device | Function |
|---|---|---|---|
| IR Capture | Digital Pin 2 | Demodulating IR Receiver (e.g., TSOP38238) | Mark/space edge detection for incoming remote signals |
| IR Verification | Digital Pin 3 | Demodulating IR Receiver (e.g., TSOP38238) | Loopback optical capture of replayed transmission |
| IR Transmitter | Digital Pin 43 | NPN Transistor Base (via 1k resistor) | 38 kHz modulated carrier output driving IR LED |
| Input Buttons | Pin 4, 5, 6 | Tactile switches to 5V (active-HIGH) | 10k pull-down resistors to GND required on each pin |
| Display | Pins 8, 9, 10, 11, 12, 13 | HD44780 16x2 LCD (RS, E, D4-D7) | Real-time status, lengths, and similarity scoring |

## Active-HIGH Pushbutton Configuration

Because the firmware checks:
```cpp
if (digitalRead(pin) == HIGH)
```
and internal pull-ups are not configured, each button line (Pin 4, 5, 6) must be connected to:
- One side to +5V
- Other side to the ATmega pin AND through a 10k ohm resistor to Ground (pull-down).
