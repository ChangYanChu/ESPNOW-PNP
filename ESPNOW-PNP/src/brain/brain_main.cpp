#include <Arduino.h>
#include "brain_config.h"
#include "brain_espnow.h"
#include "brain_tcp_server.h"
#include "gcode.h"

void setup()
{
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 5000)
        ;

    // Initialize ESP-NOW communication
    espnow_setup();

    // Initialize TCP server
    tcp_server_setup();

    // Wait for the system to stabilize
    delay(200);
}

void loop()
{
    listenToSerialStream();
    tcp_server_loop(); // Handle TCP server operations
}