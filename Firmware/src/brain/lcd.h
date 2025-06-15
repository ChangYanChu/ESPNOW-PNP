#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include "brain_config.h"

// 总是定义这些枚举类型，确保兼容性
typedef enum {
    LCD_MODE_STARTUP,      // 启动信息
    LCD_MODE_SYSTEM_INFO,  // 系统信息
    LCD_MODE_STATUS,       // 运行状态
    LCD_MODE_GCODE         // G-code命令显示
} LCDDisplayMode;

typedef enum {
    SYSTEM_INIT,          // 系统初始化中
    SYSTEM_ESPNOW_READY,   // ESP-NOW就绪
    SYSTEM_RUNNING,        // 正常运行
    SYSTEM_ERROR           // 系统错误
} SystemStatus;

#if HAS_LCD
// 有LCD硬件时的函数声明
void lcd_setup();
void lcd_update();
void lcd_show_startup();
void lcd_show_system_info();
void lcd_show_status(int online_hands, int total_hands, unsigned long uptime);
void lcd_show_error(const char* error_msg);
void lcd_show_gcode(const char* command, const char* status);
void lcd_set_mode(LCDDisplayMode mode);
void lcd_update_system_status(SystemStatus status);
void lcd_update_hand_count(int online, int total);
void lcd_update_gcode(const char* command, const char* status = "");
void triggerHeartbeatAnimation();

#else
// 没有LCD时的空实现（内联函数，不会产生额外开销）
inline void lcd_setup() {}
inline void lcd_update() {}
inline void lcd_show_startup() {}
inline void lcd_show_system_info() {}
inline void lcd_show_status(int online_hands, int total_hands, unsigned long uptime) {
    (void)online_hands; (void)total_hands; (void)uptime; // 避免未使用参数警告
}
inline void lcd_show_error(const char* error_msg) { (void)error_msg; }
inline void lcd_show_gcode(const char* command, const char* status) { 
    (void)command; (void)status; 
}
inline void lcd_set_mode(LCDDisplayMode mode) { (void)mode; }
inline void lcd_update_system_status(SystemStatus status) { (void)status; }
inline void lcd_update_hand_count(int online, int total) { (void)online; (void)total; }
inline void lcd_update_gcode(const char* command, const char* status = "") { 
    (void)command; (void)status; 
}
inline void triggerHeartbeatAnimation() {}

#endif // HAS_LCD

#endif // LCD_H