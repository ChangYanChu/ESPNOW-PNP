#include "feeder_id_manager.h"
#include "hand_config.h"
#include <EEPROM.h>

// 全局变量存储当前的Feeder ID
uint8_t currentFeederID = FEEDER_ID;

// EEPROM标识符，用于检查EEPROM是否已初始化
#define EEPROM_MAGIC_BYTE 0xAB
#define EEPROM_MAGIC_ADDR (FEEDER_ID_ADDR + 1)

void initFeederID() {
    EEPROM.begin(EEPROM_SIZE);
    
    // 检查EEPROM是否已初始化
    uint8_t magicByte = EEPROM.read(EEPROM_MAGIC_ADDR);
    
    if (magicByte == EEPROM_MAGIC_BYTE) {
        // EEPROM已初始化，读取存储的Feeder ID
        uint8_t storedID = EEPROM.read(FEEDER_ID_ADDR);
        
        // 验证读取的ID是否有效（0-49范围内）
        if (storedID < TOTAL_FEEDERS) {
            currentFeederID = storedID;
            DEBUG_PRINTF("Feeder ID loaded from EEPROM: %d\n", currentFeederID);
        } else {
            DEBUG_PRINTF("Invalid Feeder ID in EEPROM (%d), using default: %d\n", 
                         storedID, FEEDER_ID);
            currentFeederID = FEEDER_ID;
            saveFeederID(currentFeederID); // 保存默认值到EEPROM
        }
    } else {
        // EEPROM未初始化，使用默认值并保存
        DEBUG_PRINTF("EEPROM not initialized, using default Feeder ID: %d\n", FEEDER_ID);
        currentFeederID = FEEDER_ID;
        saveFeederID(currentFeederID);
    }
    
    DEBUG_PRINTF("Current Feeder ID: %d\n", currentFeederID);
}

bool saveFeederID(uint8_t feederID) {
    if (feederID >= TOTAL_FEEDERS) {
        DEBUG_PRINTF("Invalid Feeder ID: %d (must be 0-%d)\n", feederID, TOTAL_FEEDERS - 1);
        return false;
    }
    
    EEPROM.write(FEEDER_ID_ADDR, feederID);
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_BYTE);
    
    bool success = EEPROM.commit();
    if (success) {
        currentFeederID = feederID;
        DEBUG_PRINTF("Feeder ID saved to EEPROM: %d\n", feederID);
    } else {
        DEBUG_PRINTLN("Failed to save Feeder ID to EEPROM");
    }
    
    return success;
}

uint8_t getCurrentFeederID() {
    return currentFeederID;
}

void processSerialCommand() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.startsWith("SET_ID ")) {
            // 提取ID值
            String idStr = command.substring(7);
            int newID = idStr.toInt();
            
            if (newID >= 0 && newID < TOTAL_FEEDERS) {
                if (saveFeederID((uint8_t)newID)) {
                    DEBUG_PRINTF("Feeder ID changed to: %d\n", newID);
                    DEBUG_PRINTLN("Restart required for ESP-NOW registration");
                    ESP.restart(); // 重启ESP以应用新ID
                } else {
                    DEBUG_PRINTLN("Failed to save new Feeder ID");
                }
            } else {
                DEBUG_PRINTF("Invalid ID: %d (must be 0-%d)\n", newID, TOTAL_FEEDERS - 1);
            }
        }
        else if (command == "GET_ID") {
            DEBUG_PRINTF("Current Feeder ID: %d\n", getCurrentFeederID());
        }
        else if (command == "RESET_ID") {
            if (saveFeederID(FEEDER_ID)) {
                DEBUG_PRINTF("Feeder ID reset to default: %d\n", FEEDER_ID);
            }
        }
        else if (command == "HELP" || command == "?") {
            printHelp();
        }
        else if (command.length() > 0) {
            DEBUG_PRINTF("Unknown command: %s\n", command.c_str());
            DEBUG_PRINTLN("Type 'HELP' for available commands");
        }
    }
}

void printHelp() {
    DEBUG_PRINTLN("\n=== Feeder ID Manager Commands ===");
    DEBUG_PRINTLN("SET_ID <id>  - Set new Feeder ID (0-49)");
    DEBUG_PRINTLN("GET_ID       - Show current Feeder ID");
    DEBUG_PRINTLN("RESET_ID     - Reset to default Feeder ID");
    DEBUG_PRINTLN("HELP or ?    - Show this help");
    DEBUG_PRINTLN("=====================================\n");
}