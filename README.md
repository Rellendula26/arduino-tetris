# Arduino TFT Tetris

A handheld Tetris prototype built using an Arduino Nano and an Adafruit ST7735 TFT display.

## Overview

This project recreates a simplified version of Tetris on embedded hardware using SPI-based graphics rendering and physical button controls. The game logic, rendering pipeline, collision detection, and falling tetromino system were implemented directly on the microcontroller.

The project was built as an exploration into embedded systems, low-level graphics programming, and interactive hardware design.

## Features

- Real-time falling tetromino engine
- SPI TFT graphics rendering
- Physical button controls
- Collision detection
- Line clearing system
- Score tracking
- Portable handheld architecture

## Hardware

- Arduino Nano
- Adafruit 1.8" ST7735 TFT Display
- Tactile push buttons
- Breadboard prototype
- Jumper wiring
- USB/battery-powered architecture

## Software

- Arduino C++
- Adafruit GFX Library
- Adafruit ST7735 Library
- SPI communication protocol

## Challenges

Some of the biggest challenges involved:

- Debugging unstable SPI display communication
- Handling TFT initialization offsets/tab variants
- Wiring and debouncing physical button inputs
- Managing rendering performance on constrained hardware
- Building reliable game-state collision logic

## Future Improvements

- Piece rotation support
- Improved animations and effects
- Battery-powered enclosure
- 3D printed handheld shell
- Sound effects and music
- Difficulty scaling
- Start menu / game over UI

## Demo

(Add photos/videos here)

## Author

Ritvik Ellendula
