
#include "lcd.h"
#if HAS_LCD
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
static int gcode_scroll_offset = 0;                // 用于滚动显示
static unsigned long heartbeat_animation_time = 0; // 心跳动画时间
static bool heartbeat_animation_active = false;    // 心跳动画活动状态
static bool heartbeat_animation = false;           // 心跳动画状态
static bool tcp_connected = false;                 // TCP连接状态

// 自定义字符定义
// 自定义字符定义
uint8_t wifi_char[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b00100,
    0b01010,
    0b00000,
    0b00100,
    0b00000};

uint8_t ok_char[8] = {
    0b00000,
    0b00001,
    0b00011,
    0b10110,
    0b11100,
    0b01000,
    0b00000,
    0b00000};

uint8_t error_char[8] = {
    0b00000,
    0b10001,
    0b01010,
    0b00100,
    0b01010,
    0b10001,
    0b00000,
    0b00000};

// 添加心形图标
uint8_t heart_char[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000};

// TCP连接图标 - 实心圆点
uint8_t tcp_connected_char[8] = {
    0b00000,
    0b00000,
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b00000,
    0b00000};

// TCP断开图标 - 空心圆点
uint8_t tcp_disconnected_char[8] = {
    0b00000,
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b00000,
    0b00000};

void lcd_setup()
{
    lcd.init();
    lcd.backlight();

    // 创建自定义字符
    lcd.createChar(0, wifi_char);              // WiFi图标
    lcd.createChar(1, ok_char);                // 成功图标
    lcd.createChar(2, error_char);             // 错误图标
    lcd.createChar(3, heart_char);             // 心形图标
    lcd.createChar(4, tcp_connected_char);     // TCP连接图标
    lcd.createChar(5, tcp_disconnected_char);  // TCP断开图标

    startup_time = millis();
    lcd_show_startup();
}

void lcd_update()
{
    static unsigned long last_update = 0;
    unsigned long now = millis();

    // 每500ms更新一次显示
    if (now - last_update < 1000)
    {
        return;
    }
    last_update = now;

    switch (current_mode)
    {
    case LCD_MODE_STARTUP:
        // 启动显示3秒后自动切换
        if (now - startup_time > 3000)
        {
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
        if (now - gcode_display_time > 5000)
        {
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
    // 使用静态变量记录上次显示的内容，避免不必要的更新
    static unsigned long last_uptime = 0;
    static bool last_heartbeat = false;

    // 只清除并更新第一行
    lcd.setCursor(0, 0);
    lcd.print("                "); // 清除第一行

    lcd.setCursor(0, 0);
    lcd.print("Feeder:");
    lcd.print(online);
    lcd.print("/");
    lcd.print(total);

    // 显示连接状态指示
    lcd.setCursor(12, 0);
    if (online == total && total > 0)
    {
        lcd.write(1); // 全部在线 - OK图标
    }
    else if (online > 0)
    {
        lcd.print("~"); // 部分在线
    }
    else
    {
        lcd.write(2); // 全部离线 - 错误图标
    }

    // TCP连接状态显示（在第13位置）- 每次都更新显示
    lcd.setCursor(13, 0);
    if (tcp_connected)
    {
        lcd.write(4); // TCP连接图标 - 实心圆点
    }
    else
    {
        lcd.write(5); // TCP断开图标 - 空心圆点
    }

    // 心跳动画（在最后一个位置）- 独立更新
    lcd.setCursor(15, 0);
    if (online > 0)
    {
        // 每500ms切换一次心跳动画
        static unsigned long last_heartbeat_anim = 0;
        unsigned long now = millis();
        if (now - last_heartbeat_anim > 500)
        {
            heartbeat_animation = !heartbeat_animation;
            last_heartbeat_anim = now;
        }

        // 只在心跳状态改变时更新
        if (last_heartbeat != heartbeat_animation)
        {
            if (heartbeat_animation)
            {
                lcd.write(3); // 使用心形图标
            }
            else
            {
                lcd.print(" ");
            }
            last_heartbeat = heartbeat_animation;
        }
    }
    else
    {
        if (last_heartbeat != false)
        {
            lcd.print(" "); // 没有在线设备时不显示动画
            last_heartbeat = false;
        }
    }

    // 检查是否需要更新第二行（运行时间）
    // 只在秒数变化时更新，减少更新频率
    if (last_uptime != uptime)
    {
        lcd.setCursor(0, 1);
        lcd.print("                "); // 清除第二行

        lcd.setCursor(0, 1);
        lcd.print("Up:");

        // 显示运行时间 (格式: XXh XXm 或 XXXXs)
        if (uptime >= 3600)
        {
            lcd.print(uptime / 3600);
            lcd.print("h ");
            lcd.print((uptime % 3600) / 60);
            lcd.print("m");
        }
        else if (uptime >= 60)
        {
            lcd.print(uptime / 60);
            lcd.print("m ");
            lcd.print(uptime % 60);
            lcd.print("s");
        }
        else
        {
            lcd.print(uptime);
            lcd.print("s");
        }

        last_uptime = uptime;
    }
}

void lcd_show_error(const char *error_msg)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(2); // 错误图标
    lcd.print(" ERROR");

    lcd.setCursor(0, 1);
    String msg = String(error_msg);
    if (msg.length() > 16)
    {
        lcd.print(msg.substring(0, 16));
    }
    else
    {
        lcd.print(msg);
    }
}

void lcd_show_gcode(const char *command, const char *status)
{
    // 清屏并显示G-code信息
    lcd.clear();

    if (strlen(status) > 0)
    {
        // 显示状态信息
        lcd.setCursor(0, 0);

        // 检查是否是Feed相关的状态消息
        String statusStr = String(status);

        // 处理包含 "Feed N" 的状态消息
        if (statusStr.indexOf("Feed N") >= 0)
        {
            // 显示Feed状态，如果超过16字符则滚动显示
            if (statusStr.length() > 16)
            {
                // 实现滚动显示
                static unsigned long last_scroll_time = 0;
                static int scroll_offset = 0;
                unsigned long now = millis();

                if (now - last_scroll_time > 800) // 每800ms滚动一次
                {
                    if (scroll_offset + 16 >= statusStr.length())
                    {
                        scroll_offset = 0; // 重置滚动
                    }
                    else
                    {
                        scroll_offset++;
                    }
                    last_scroll_time = now;
                }

                String displayText = statusStr.substring(scroll_offset, scroll_offset + 16);
                lcd.print(displayText);

                // 在右下角显示滚动指示器
                lcd.setCursor(15, 1);
                lcd.print(">");
            }
            else
            {
                lcd.print(statusStr);
            }
        }
        else
        {
            // 普通状态消息
            if (strlen(status) <= 16)
            {
                lcd.print(status);
            }
            else
            {
                // 如果状态消息太长，显示前16个字符
                String shortStatus = String(status).substring(0, 16);
                lcd.print(shortStatus);
            }
        }

        // 第二行显示命令（如果有）
        if (strlen(command) > 0)
        {
            lcd.setCursor(0, 1);
            if (statusStr.length() <= 16) // 第一行没有滚动时才显示命令
            {
                lcd.print("Cmd: ");
                if (strlen(command) <= 11)
                {
                    lcd.print(command);
                }
                else
                {
                    String shortCommand = String(command).substring(0, 11);
                    lcd.print(shortCommand);
                }
            }
        }
    }
    else if (strlen(command) > 0)
    {
        // 只显示命令
        lcd.setCursor(0, 0);
        lcd.print("G-Code:");
        lcd.setCursor(0, 1);
        if (strlen(command) <= 16)
        {
            lcd.print(command);
        }
        else
        {
            String shortCommand = String(command).substring(0, 16);
            lcd.print(shortCommand);
        }
    }
    else
    {
        // 没有命令和状态时的默认显示
        lcd.setCursor(0, 0);
        lcd.print("G-Code Mode");
        lcd.setCursor(0, 1);
        lcd.print("Ready...");
    }
}

void lcd_set_mode(LCDDisplayMode mode)
{
    current_mode = mode;

    // 根据模式立即显示相应内容
    switch (mode)
    {
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
    switch (status)
    {
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

void lcd_update_gcode(const char *command, const char *status)
{
    current_gcode = String(command);
    gcode_status = String(status);
    gcode_display_time = millis();
    gcode_scroll_offset = 0; // 重置滚动偏移量

    // 自动切换到G-code显示模式
    if (current_mode == LCD_MODE_STATUS || current_mode == LCD_MODE_GCODE)
    {
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
    if (current_mode == LCD_MODE_STATUS)
    {
        lcd.setCursor(15, 0); // 心跳指示器位置 (最后一列)
        lcd.write(3);         // 显示心形图标
    }
}

void lcd_update_tcp_status(bool connected)
{
    tcp_connected = connected;
    Serial.printf("LCD TCP status updated: %s\n", connected ? "Connected" : "Disconnected");
    
    // 如果在状态显示模式，立即更新TCP状态指示器
    if (current_mode == LCD_MODE_STATUS)
    {
        lcd.setCursor(13, 0); // TCP状态指示器位置
        if (connected)
        {
            lcd.write(4); // TCP连接图标 - 实心圆点
            Serial.println("LCD: Displaying solid circle for TCP connected");
        }
        else
        {
            lcd.write(5); // TCP断开图标 - 空心圆点
            Serial.println("LCD: Displaying hollow circle for TCP disconnected");
        }
    }
}

#endif // HAS_LCD