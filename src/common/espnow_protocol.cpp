#include "espnow_protocol.h"

// 计算校验和 (简单的16位校验和)
uint16_t calculateChecksum(const ESPNowPacket* packet) {
    uint16_t checksum = 0;
    const uint8_t* data = (const uint8_t*)packet;
    
    // 计算除checksum字段外的所有字节
    for (size_t i = 0; i < sizeof(ESPNowPacket) - sizeof(uint16_t); i++) {
        checksum += data[i];
    }
    
    return checksum;
}

// 验证校验和
bool verifyChecksum(const ESPNowPacket* packet) {
    uint16_t calculated = calculateChecksum(packet);
    return calculated == packet->checksum;
}

// 设置数据包校验和
void setPacketChecksum(ESPNowPacket* packet) {
    packet->checksum = calculateChecksum(packet);
}
