#include <Arduino.h>
#include <PacketSerial.h>
#include "serial.h"

PacketSerial_<SLIP, 0xc0, 1024> serialConnection;

void serialSend(const uint8_t* buffer, size_t size) {
  serialConnection.send(buffer, size);
}
void serialBegin(unsigned long speed) {
  serialConnection.begin(speed);
}
void serialUpdate() {
  serialConnection.update();
}
void serialSetPacketHandler(PacketHandlerFunction handler) {
  serialConnection.setPacketHandler(handler);
}
