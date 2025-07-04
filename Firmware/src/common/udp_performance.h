#ifndef UDP_PERFORMANCE_H
#define UDP_PERFORMANCE_H

#include <Arduino.h>

// =============================================================================
// UDP性能监控和优化
// =============================================================================

// 性能统计结构
struct UDPPerformanceStats {
    uint32_t packetsPerSecond;          // 每秒包数
    uint32_t bytesPerSecond;            // 每秒字节数
    uint32_t averageLatency;            // 平均延迟(ms)
    uint32_t packetLossRate;            // 丢包率(百分比)
    uint32_t maxLatency;                // 最大延迟
    uint32_t minLatency;                // 最小延迟
    uint32_t jitter;                    // 抖动
};

// 性能监控类
class UDPPerformanceMonitor {
private:
    uint32_t lastStatsTime;
    uint32_t packetCount;
    uint32_t byteCount;
    uint32_t latencySum;
    uint32_t latencyCount;
    uint32_t maxLat;
    uint32_t minLat;
    
public:
    UDPPerformanceMonitor();
    
    // 记录发送的包
    void recordSentPacket(size_t bytes, uint32_t timestamp);
    
    // 记录接收的包
    void recordReceivedPacket(size_t bytes, uint32_t sendTimestamp);
    
    // 获取性能统计
    UDPPerformanceStats getStats();
    
    // 重置统计
    void resetStats();
    
    // 打印性能报告
    void printPerformanceReport();
    
    // 自动优化建议
    void suggestOptimizations();
};

// 全局性能监控实例
// extern UDPPerformanceMonitor g_perfMonitor;  // 暂未实现

// =============================================================================
// 性能优化工具函数
// =============================================================================

// 自适应调整心跳间隔
void adaptiveHeartbeatInterval();

// 动态调整缓冲区大小
void optimizeBufferSizes();

// 网络质量检测
uint8_t getNetworkQuality();

// 自动重连优化
void optimizeReconnectionStrategy();

#endif // UDP_PERFORMANCE_H
