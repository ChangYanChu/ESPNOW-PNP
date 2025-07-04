#include "udp_protocol.h"

// =============================================================================
// UDP工具函数实现
// =============================================================================

// 全局序列号计数器
static uint32_t g_sequenceCounter = 0;

// 生成序列号
uint32_t generateSequence() {
    return ++g_sequenceCounter;
}

// 获取当前时间戳
uint32_t getCurrentTimestamp() {
    return millis();
}

// 验证UDP包合法性
bool isValidUDPPacket(const uint8_t* data, size_t len, UDPPacketType expectedType) {
    if (!data || len < 1) {
        return false;
    }
    
    UDPPacketType packetType = (UDPPacketType)data[0];
    
    // 检查包类型
    if (packetType != expectedType) {
        return false;
    }
    
    // 检查包长度
    switch (packetType) {
        case UDP_PKT_DISCOVERY_REQUEST:
            return len >= sizeof(UDPDiscoveryRequest);
        case UDP_PKT_DISCOVERY_RESPONSE:
            return len >= sizeof(UDPDiscoveryResponse);
        case UDP_PKT_COMMAND:
            return len >= sizeof(UDPCommandPacket);
        case UDP_PKT_RESPONSE:
            return len >= sizeof(UDPResponsePacket);
        case UDP_PKT_HEARTBEAT:
            return len >= sizeof(UDPHeartbeatPacket);
        case UDP_PKT_PING:
            return len >= 1;  // Ping包只需要类型字段
        default:
            return false;
    }
}

// 打印UDP包信息 (调试用)
void printUDPPacket(const uint8_t* data, size_t len, bool isIncoming) {
    if (!data || len < 1) {
        Serial.println("[UDP] 无效数据包");
        return;
    }
    
    UDPPacketType packetType = (UDPPacketType)data[0];
    const char* direction = isIncoming ? "接收" : "发送";
    
    Serial.printf("[UDP] %s 包类型=0x%02X 长度=%d ", direction, packetType, len);
    
    switch (packetType) {
        case UDP_PKT_DISCOVERY_REQUEST:
            Serial.println("(发现请求)");
            break;
        case UDP_PKT_DISCOVERY_RESPONSE:
            Serial.println("(发现响应)");
            break;
        case UDP_PKT_COMMAND:
            if (len >= sizeof(UDPCommandPacket)) {
                UDPCommandPacket* pkt = (UDPCommandPacket*)data;
                Serial.printf("(命令) seq=%u cmd=0x%02X\n", pkt->sequence, pkt->command.commandType);
            } else {
                Serial.println("(命令-长度不足)");
            }
            break;
        case UDP_PKT_RESPONSE:
            if (len >= sizeof(UDPResponsePacket)) {
                UDPResponsePacket* pkt = (UDPResponsePacket*)data;
                Serial.printf("(响应) seq=%u status=0x%02X\n", pkt->sequence, pkt->response.status);
            } else {
                Serial.println("(响应-长度不足)");
            }
            break;
        case UDP_PKT_HEARTBEAT:
            Serial.println("(心跳)");
            break;
        case UDP_PKT_PING:
            Serial.println("(Ping)");
            break;
        default:
            Serial.printf("(未知类型=0x%02X)\n", packetType);
            break;
    }
}

// =============================================================================
// 性能优化工具函数实现
// =============================================================================

// 获取时间戳低16位(减少网络传输)
uint16_t getTimestampLow() {
    return (uint16_t)(millis() & 0xFFFF);
}

// 检查包是否新鲜(基于低16位时间戳)
bool isPacketFresh(uint16_t timestampLow, uint32_t maxAge) {
    uint16_t currentLow = getTimestampLow();
    
    // 处理时间戳回绕
    uint16_t diff;
    if (currentLow >= timestampLow) {
        diff = currentLow - timestampLow;
    } else {
        diff = (0xFFFF - timestampLow) + currentLow + 1;
    }
    
    return diff < (maxAge & 0xFFFF);
}

// 优化WiFi设置以提高性能
void optimizeWiFiSettings() {
#if defined ESP32
    // ESP32特定优化
    WiFi.setSleep(false);                    // 禁用WiFi休眠
    WiFi.setTxPower(WIFI_POWER_19_5dBm);    // 设置合适的发射功率
#elif defined ESP8266
    // ESP8266特定优化
    WiFi.setSleep(false);                    // 禁用WiFi休眠
    WiFi.setOutputPower(17);                 // 设置合适的发射功率
    WiFi.setPhyMode(WIFI_PHY_MODE_11N);     // 使用802.11n模式
#endif
}

// 设置UDP缓冲区大小以优化性能
void setUDPBufferSizes() {
    // 这个函数在各个UDP对象初始化时调用
    // 具体实现在各自的setup函数中
}
