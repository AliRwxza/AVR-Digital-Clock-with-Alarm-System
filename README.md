# AVR Digital Clock with Alarm System

An ATmega-based digital clock implementation featuring 7-segment display, LCD interface, and multiple alarm capabilities with message support.

## Features

- Real-time clock display on 7-segment display (HH:MM:SS)
- LCD interface for system interaction
- Support for up to 4 configurable alarms
- Custom alarm messages using keypad input
- Alarm snooze and dismissal options
- EEPROM storage for alarm persistence
- Dynamic LCD animations

## Proteus Design

Proteus design consists of the following components:

- ATmega32 microcontroller (8MHz)
- 16x2 LCD display (LM016L)
- 4x4 keypad matrix
- 6-digit 7-segment display (7SEG-MPX6-CA)
- Buzzer for alarm
- EEPROM for storage

## Proteus Representation

![image](https://github.com/user-attachments/assets/8f3dd850-c3f5-467a-b780-abfd27f9a6f5)

## Controls

### Keypad Functions
- `A`: Configure new alarm
- `B`: Display/manage alarms
- `C`: Set clock time
- `D`: Confirm/Exit
- `0-9`: Numeric input
- `*/#`: Additional characters for messages

### Alarm Management
- During alarm:
  - `A`: Turn off alarm
  - `B`: Snooze (2 minutes)
- In alarm list:
  - `1-4`: Delete corresponding alarm
  - `D`: Exit view

## Message Input System
Characters are mapped to keypad buttons as follows:
```
7: a    8: b    9: c    A: .
4: d    5: e    6: f    B: ?
1: g    2: h    3: i    C: backspace
*: j    0: k    #: l    D: confirm
```
