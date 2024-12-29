#ifndef serial_h
#define serial_h

typedef void (*PacketHandlerFunction)(const uint8_t* buffer, size_t size);

void serialBegin(unsigned long speed);
void serialUpdate();
void serialSend(const uint8_t* buffer, size_t size);
void serialSetPacketHandler(PacketHandlerFunction handler);
void debug(String str);
#endif
