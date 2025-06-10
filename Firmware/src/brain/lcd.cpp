
#include "lcd.h"
#include "brain_config.h"
#include <LCDI2C_Multilingual_MCD.h>
#include <WiFi.h>

// LCD对象 - 修改为1602 (16列2行)
LCDI2C_Generic lcd(0x27, 16, 2); 

// 全局状态变量
static LCDDisplayMode current_mode = LCD_MODE_STARTUP;
static SystemStatus system_status = SYSTEM_INIT;
static int online_hands = 0;
static int total_hands = 0;
static unsigned long startup_time = 0;
static String error_message = "";
static String current_gcode = "";
static String gcode_status = "";
static unsigned long gcode_display_time = 0;
static int gcode_scroll_offset = 0;  // 用于滚动显示
static unsigned long heartbeat_animation_time = 0;  // 心跳动画时间
static bool heartbeat_animation_active = false;     // 心跳动画活动状态
static bool heartbeat_animation = false;  // 心跳动画状态

// 自定义字符定义
uint8_t wifi_char[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b00100,
    0b01010,
    0b00000,
    0b00100,
    0b00000
};

uint8_t ok_char[8] = {
    0b00000,
    0b00001,
    0b00011,
    0b10110,
    0b11100,
    0b01000,
    0b00000,
    0b00000
};

uint8_t error_char[8] = {
    0b00000,
    0b10001,
    0b01010,
    0b00100,
    0b01010,
    0b10001,
    0b00000,
    0b00000
};

void lcd_setup()
{
    lcd.init();
    lcd.backlight();
    
    // 创建自定义字符
    lcd.createChar(0, wifi_char);  // WiFi图标
    lcd.createChar(1, ok_char);    // 成功图标
    lcd.createChar(2, error_char); // 错误图标
    
    startup_time = millis();
    lcd_show_startup();
}

void lcd_update()
{
    static unsigned long last_update = 0;
    unsigned long now = millis();
    
    // 每500ms更新一次显示
    if (now - last_update < 500) {
        return;
    }
    last_update = now;
    
    switch (current_mode) {
        case LCD_MODE_STARTUP:
            // 启动显示3秒后自动切换
            if (now - startup_time > 3000) {
                lcd_set_mode(LCD_MODE_SYSTEM_INFO);
            }
            break;
            
        case LCD_MODE_SYSTEM_INFO:
            lcd_show_system_info();
            break;
            
        case LCD_MODE_STATUS:
        {
            unsigned long uptime = (now - startup_time) / 1000;
            lcd_show_status(online_hands, total_hands, uptime);
            break;
        }
            
        case LCD_MODE_GCODE:
            lcd_show_gcode(current_gcode.c_str(), gcode_status.c_str());
            // G-code显示5秒后自动切换回状态显示
            if (now - gcode_display_time > 5000) {
                lcd_set_mode(LCD_MODE_STATUS);
            }
            break;
    }
}

void lcd_show_startup()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESP-NOW PNP");
    lcd.setCursor(0, 1);
    lcd.print("Brain v");
    lcd.print(BRAIN_VERSION);
    lcd.write(1); // 显示OK图标
}

void lcd_show_system_info()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESP-NOW Ready");
    lcd.write(1); // OK图标
    
    lcd.setCursor(0, 1);
    lcd.print("MAC:");
    String mac = WiFi.macAddress();
    lcd.print(mac.substring(12)); // 显示后4位MAC
}

void lcd_show_status(int online, int total, unsigned long uptime)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hands:");
    lcd.print(online);
    lcd.print("/");
    lcd.print(total);
    
    // 显示连接状态指示和心跳动画
    lcd.setCursor(12, 0);
    if (online == total && total > 0) {
        lcd.write(1); // 全部在线 - OK图标
    } else if (online > 0) {
        lcd.print("~"); // 部分在线
    } else {
        lcd.write(2); // 全部离线 - 错误图标
    }
    
    // 心跳动画（在最后一个位置）
    lcd.setCursor(15, 0);
    if (online > 0) {
        // 每500ms切换一次心跳动画
        static unsigned long last_heartbeat_anim = 0;
        unsigned long now = millis();
        if (now - last_heartbeat_anim > 500) {
            heartbeat_animation = !heartbeat_animation;
            last_heartbeat_anim = now;
        }
        lcd.print(heartbeat_animation ? "*" : " ");
    } else {
        lcd.print(" "); // 没有在线设备时不显示动画
    }
    
    lcd.setCursor(0, 1);
    lcd.print("Up:");
    
    // 显示运行时间 (格式: XXh XXm 或 XXXXs)
    if (uptime >= 3600) {
        lcd.print(uptime / 3600);
        lcd.print("h ");
        lcd.print((uptime % 3600) / 60);
        lcd.print("m");
    } else if (uptime >= 60) {
        lcd.print(uptime / 60);
        lcd.print("m ");
        lcd.print(uptime % 60);
        lcd.print("s");
    } else {
        lcd.print(uptime);
        lcd.print("s");
    }
}

void lcd_show_error(const char* error_msg)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(2); // 错误图标
    lcd.print(" ERROR");
    
    lcd.setCursor(0, 1);
    String msg = String(error_msg);
    if (msg.length() > 16) {
        lcd.print(msg.substring(0, 16));
    } else {
        lcd.print(msg);
    }
}

void lcd_show_gcode(const char* command, const char* status)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    
    // 显示G-code命令，如果太长则截断
    String cmd = String(command);
    if (cmd.length() > 16) {
        lcd.print(cmd.substring(0, 16));
    } else {
        lcd.print(cmd);
    }
    
    lcd.setCursor(0, 1);
    
    // 显示状态 - 针对常见的长消息进行缩写
    if (strlen(status) > 0) {
        String stat = String(status);
        
        // 针对常见的长消息进行缩写处理
        if (stat.indexOf("Feeder advance command completed") != -1) {
            lcd.write(1); // OK图标
            lcd.print(" Feed Done");
        } else if (stat.indexOf("Feeder command timeout") != -1) {
            lcd.write(2); // 错误图标
            lcd.print(" Timeout");
        } else if (stat.indexOf("Another feeder command is still in progress") != -1) {
            lcd.write(2); // 错误图标
            lcd.print(" Busy");
        } else if (stat.indexOf("Failed to send feeder advance command") != -1) {
            lcd.write(2); // 错误图标
            lcd.print(" Send Fail");
        } else if (stat.startsWith("ok")) {
            lcd.write(1); // OK图标
            lcd.print(" ");
            String remaining = stat.substring(3); // 跳过"ok "
            if (remaining.length() > 14) {
                lcd.print(remaining.substring(0, 14));
            } else {
                lcd.print(remaining);
            }
        } else if (stat.startsWith("error")) {
            lcd.write(2); // 错误图标
            lcd.print(" ");
            String remaining = stat.substring(6); // 跳过"error "
            
            // 对特定错误消息进行缩写处理
            if (remaining.indexOf("Enable feeder first!") != -1) {
                lcd.print("Enable M610 S1");
            } else if (remaining.length() > 14) {
                lcd.print(remaining.substring(0, 14));
            } else {
                lcd.print(remaining);
            }
        } else {
            // 其他状态 - 如果太长则滚动显示
            if (stat.length() > 16) {
                static unsigned long last_scroll = 0;
                unsigned long now = millis();
                
                // 每1秒滚动一次
                if (now - last_scroll > 1000) {
                    gcode_scroll_offset++;
                    if (gcode_scroll_offset > stat.length() - 16) {
                        gcode_scroll_offset = 0; // 重新开始
                    }
                    last_scroll = now;
                }
                
                lcd.print(stat.substring(gcode_scroll_offset, gcode_scroll_offset + 16));
            } else {
                lcd.print(stat);
            }
        }
    } else {
        lcd.print("Processing...");
    }
}

void lcd_set_mode(LCDDisplayMode mode)
{
    current_mode = mode;
    
    // 根据模式立即显示相应内容
    switch (mode) {
        case LCD_MODE_STARTUP:
            lcd_show_startup();
            break;
        case LCD_MODE_SYSTEM_INFO:
            lcd_show_system_info();
            break;
        case LCD_MODE_STATUS:
        {
            unsigned long uptime = (millis() - startup_time) / 1000;
            lcd_show_status(online_hands, total_hands, uptime);
            break;
        }
        case LCD_MODE_GCODE:
            lcd_show_gcode(current_gcode.c_str(), gcode_status.c_str());
            break;
    }
}

void lcd_update_system_status(SystemStatus status)
{
    system_status = status;
    
    // 根据状态自动切换显示模式
    switch (status) {
        case SYSTEM_ESPNOW_READY:
        case SYSTEM_RUNNING:
            lcd_set_mode(LCD_MODE_STATUS);
            break;
        case SYSTEM_ERROR:
            lcd_show_error(error_message.c_str());
            break;
    }
}

void lcd_update_hand_count(int online, int total)
{
    online_hands = online;
    total_hands = total;
}

void lcd_update_gcode(const char* command, const char* status)
{
    current_gcode = String(command);
    gcode_status = String(status);
    gcode_display_time = millis();
    gcode_scroll_offset = 0;  // 重置滚动偏移量
    
    // 自动切换到G-code显示模式
    if (current_mode == LCD_MODE_STATUS || current_mode == LCD_MODE_GCODE) {
        lcd_set_mode(LCD_MODE_GCODE);
    }
}

void triggerHeartbeatAnimation()
{
    // 触发心跳动画 - 激活动画并设置时间
    heartbeat_animation_active = true;
    heartbeat_animation_time = millis();
    heartbeat_animation = true;
    
    // 如果当前在状态显示模式，立即更新心跳指示器位置
    if (current_mode == LCD_MODE_STATUS) {
        lcd.setCursor(15, 0);  // 心跳指示器位置 (最后一列)
        lcd.print("*");        // 显示心跳指示器
    }
}