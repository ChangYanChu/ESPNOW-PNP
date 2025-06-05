# ESPNOW-PNP Project

## Overview
The ESPNOW-PNP project is designed to facilitate communication between devices using ESP-NOW and TCP protocols. It includes functionalities for G-code processing, enabling users to control feeders and other components through G-code commands.

## Project Structure
```
ESPNOW-PNP
├── src
│   ├── brain
│   │   ├── brain_main.cpp          # Entry point of the application, initializes serial and ESP-NOW communication.
│   │   ├── brain_config.h           # Configuration definitions including version, baud rate, and feeder settings.
│   │   ├── brain_espnow.cpp         # Implements ESP-NOW communication functionalities.
│   │   ├── brain_espnow.h           # Header file for ESP-NOW related functions and variables.
│   │   ├── brain_tcp_server.cpp      # Implements TCP server functionalities for G-code communication.
│   │   ├── brain_tcp_server.h        # Header file for TCP server related functions and variables.
│   │   ├── gcode.cpp                # Implements G-code command parsing and handling.
│   │   └── gcode.h                  # Header file for G-code processing functions and variables.
│   ├── hand
│   │   └── [existing hand files]    # Contains files related to hand functionalities.
│   └── common
│       ├── espnow_protocol.h         # Defines ESP-NOW protocol related data structures and constants.
│       └── tcp_protocol.h            # Defines TCP protocol related data structures and constants.
├── platformio.ini                   # PlatformIO configuration file defining build environment and dependencies.
└── README.md                        # Documentation and project description.
```

## Features
- **ESP-NOW Communication**: Enables low-power, peer-to-peer communication between ESP devices.
- **TCP Server**: Listens for incoming TCP connections and processes G-code commands.
- **G-code Processing**: Parses and executes G-code commands to control feeders and other components.

## Getting Started
1. Clone the repository to your local machine.
2. Open the project in your preferred IDE.
3. Install the necessary dependencies using PlatformIO.
4. Upload the code to your ESP device.
5. Connect to the TCP server using a client to send G-code commands.

## Usage
- Use the TCP server to send G-code commands for controlling feeders.
- Monitor the serial output for debugging and status messages.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.