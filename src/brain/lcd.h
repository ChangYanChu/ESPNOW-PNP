#ifndef LCD_H
#define LCD_H

#include <Arduino.h>

// LCD显示模式枚举
typedef enum {
    LCD_MODE_STARTUP,      // 启动信息
    LCD_MODE_SYSTEM_INFO,  // 系统信息
    LCD_MODE_STATUS,       // 运行状态
    LCD_MODE_GCODE         // G-code命令显示
} LCDDisplayMode;

// 系统状态枚举
typedef enum {
    SYSTEM_INIT,          // 系统初始化中
    SYSTEM_ESPNOW_READY,   // ESP-NOW就绪
    SYSTEM_RUNNING,        // 正常运行
    SYSTEM_ERROR           // 系统错误
} SystemStatus;

// LCD管理函数
void lcd_setup();
void lcd_update();
void lcd_show_startup();
void lcd_show_system_info();
void lcd_show_status(int online_hands, int total_hands, unsigned long uptime);
void lcd_show_error(const char* error_msg);
void lcd_show_gcode(const char* command, const char* status);
void lcd_set_mode(LCDDisplayMode mode);

// 状态更新函数
void lcd_update_system_status(SystemStatus status);
void lcd_update_hand_count(int online, int total);
void lcd_update_gcode(const char* command, const char* status = "");
void triggerHeartbeatAnimation();
void triggerHeartbeatAnimation();

#endif // LCD_H