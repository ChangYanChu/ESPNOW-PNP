#include "feeder.h"

bool FeederClass::isInitialized() {
    return (this->feederNo != -1);
}

void FeederClass::initialize(uint8_t _feederNo) {
    this->feederNo = _feederNo;
}

#ifdef HAS_FEEDBACKLINES
bool FeederClass::hasFeedbackLine() {
    if(feederFeedbackPinMap[this->feederNo] != -1) {
        return true;
    } else {
        return false;
    }
}
#else
bool FeederClass::hasFeedbackLine() {
    return false;
}
#endif

void FeederClass::outputCurrentSettings() {
    Serial.print("M");
    Serial.print(MCODE_UPDATE_FEEDER_CONFIG);
    Serial.print(" N");
    Serial.print(this->feederNo);
    Serial.print(" A");
    Serial.print(this->feederSettings.full_advanced_angle);
    Serial.print(" B");
    Serial.print(this->feederSettings.half_advanced_angle);
    Serial.print(" C");
    Serial.print(this->feederSettings.retract_angle);
    Serial.print(" F");
    Serial.print(this->feederSettings.feed_length);
    Serial.print(" U");
    Serial.print(this->feederSettings.time_to_settle);
    Serial.print(" V");
    Serial.print(this->feederSettings.motor_min_pulsewidth);
    Serial.print(" W");
    Serial.print(this->feederSettings.motor_max_pulsewidth);
#ifdef HAS_FEEDBACKLINES
    Serial.print(" X");
    Serial.print(this->feederSettings.ignore_feedback);
#endif
    Serial.println();
}

void FeederClass::setup() {
    // 加载设置
    this->loadFeederSettings();

    // 初始化反馈输入
#ifdef HAS_FEEDBACKLINES
    if(this->hasFeedbackLine()) {
        pinMode((uint8_t)feederFeedbackPinMap[this->feederNo], INPUT_PULLUP);
        this->lastButtonState = digitalRead(feederFeedbackPinMap[this->feederNo]);
    }
#endif

    // 设置初始位置
    this->gotoRetractPositionSetup();
}

FeederClass::sFeederSettings FeederClass::getSettings() {
    return this->feederSettings;
}

void FeederClass::setSettings(sFeederSettings UpdatedFeederSettings) {
    this->feederSettings = UpdatedFeederSettings;

    #ifdef DEBUG
    Serial.println(F("Updated feeder settings"));
    this->outputCurrentSettings();
    #endif
}

void FeederClass::loadFeederSettings() {
    // 使用Preferences替代EEPROM
    String namespace_name = String(PREFS_NAMESPACE) + String(this->feederNo);
    prefs.begin(namespace_name.c_str(), true); // 只读模式
    
    // 检查版本
    String version = prefs.getString(PREFS_KEY_VERSION, "");
    if (version != CONFIG_VERSION) {
        // 版本不匹配，使用默认设置
        prefs.end();
        factoryReset();
        return;
    }
    
    // 读取设置
    size_t bytesRead = prefs.getBytes(PREFS_KEY_FEEDER, &this->feederSettings, sizeof(this->feederSettings));
    prefs.end();
    
    if (bytesRead != sizeof(this->feederSettings)) {
        // 读取失败，使用默认设置
        factoryReset();
    }

    #ifdef DEBUG
    Serial.println(F("Loaded settings from preferences:"));
    this->outputCurrentSettings();
    #endif
}

void FeederClass::saveFeederSettings() {
    // 使用Preferences替代EEPROM
    String namespace_name = String(PREFS_NAMESPACE) + String(this->feederNo);
    prefs.begin(namespace_name.c_str(), false); // 读写模式
    
    // 保存版本
    prefs.putString(PREFS_KEY_VERSION, CONFIG_VERSION);
    
    // 保存设置
    prefs.putBytes(PREFS_KEY_FEEDER, &this->feederSettings, sizeof(this->feederSettings));
    prefs.end();

    #ifdef DEBUG
    Serial.println(F("Stored settings to preferences:"));
    this->outputCurrentSettings();
    #endif
}

void FeederClass::factoryReset() {
    // 恢复默认设置并保存
    this->saveFeederSettings();
}

void FeederClass::attachServo() {
    // 连接舵机并设置脉宽范围
    this->servo.attach(feederPinMap[this->feederNo], 
                       this->feederSettings.motor_min_pulsewidth,
                       this->feederSettings.motor_max_pulsewidth);
}

void FeederClass::detachServo() {
    // 断开舵机连接以节省电力
    this->servo.detach();
}

void FeederClass::gotoPostPickPosition() {
    if(this->feederPosition == sAT_FULL_ADVANCED_POSITION) {
        this->gotoRetractPosition();
        #ifdef DEBUG
        Serial.println("gotoPostPickPosition retracted feeder");
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("gotoPostPickPosition didn't need to retract feeder");
        #endif
    }
}

void FeederClass::gotoRetractPosition() {
    this->attachServo();
    this->servo.write(this->feederSettings.retract_angle);
    
    this->feederPosition = sAT_RETRACT_POSITION;
    this->feederState = sMOVING;
    this->lastTimePositionChange = millis();
    
    #ifdef DEBUG
    Serial.println("going to retract now");
    #endif
}

void FeederClass::gotoRetractPositionSetup() {
    // 启动时不激活舵机
    this->feederPosition = sAT_RETRACT_POSITION;
    this->feederState = sIDLE;
    
    #ifdef DEBUG
    Serial.println("feeder initialized at retract position");
    #endif
}

void FeederClass::gotoHalfAdvancedPosition() {
    this->attachServo();
    this->servo.write(this->feederSettings.half_advanced_angle);
    
    this->feederPosition = sAT_HALF_ADVANCED_POSITION;
    this->feederState = sMOVING;
    this->lastTimePositionChange = millis();
    
    #ifdef DEBUG
    Serial.println("going to half adv now");
    #endif
}

void FeederClass::gotoFullAdvancedPosition() {
    this->attachServo();
    this->servo.write(this->feederSettings.full_advanced_angle);

    this->feederPosition = sAT_FULL_ADVANCED_POSITION;
    this->feederState = sMOVING;
    this->lastTimePositionChange = millis();
    
    #ifdef DEBUG
    Serial.println("going to full adv now");
    #endif
}

void FeederClass::gotoAngle(uint8_t angle) {
    this->attachServo();
    this->servo.write(angle);
    
    #ifdef DEBUG
    Serial.print("going to ");
    Serial.print(angle);
    Serial.println("deg");
    #endif
}

bool FeederClass::advance(uint8_t feedLength, bool overrideError) {
    #ifdef DEBUG
    Serial.println(F("advance triggered"));
    Serial.println(this->reportFeederErrorState());
    #endif
    
    // 检查喂料器状态
    if (!this->feederIsOk()) {
        if (!overrideError) {
            return false;
        } else {
            #ifdef DEBUG
            Serial.println(F("overridden error temporarily"));
            #endif
        }
    }

    // 检查喂料长度
    if (feedLength == 0) {
        #ifdef DEBUG
        Serial.println(F("advance ignored, 0 feedlength was given"));
        #endif
        return true;
    } else if (feedLength > 0 && this->feederState != sIDLE) {
        #ifdef DEBUG
        Serial.print(F("advance ignored, feederState!=sIDLE"));
        Serial.print(F(" (feederState="));
        Serial.print(this->feederState);
        Serial.println(F(")"));
        #endif
        return false;
    } else {
        #ifdef DEBUG
        Serial.print(F("advance initialized, remainingFeedLength="));
        Serial.println(feedLength);
        #endif
        this->remainingFeedLength = feedLength;
    }

    return true;
}

bool FeederClass::feederIsOk() {
    return (this->getFeederErrorState() != sERROR);
}

FeederClass::tFeederErrorState FeederClass::getFeederErrorState() {
#ifdef HAS_FEEDBACKLINES
    if(!this->hasFeedbackLine()) {
        return sOK_NOFEEDBACKLINE;
    }

    if(digitalRead((uint8_t)feederFeedbackPinMap[this->feederNo]) == LOW) {
        return sOK;
    } else {
        if(this->feederSettings.ignore_feedback == 1) {
            return sERROR_IGNORED;
        } else {
            return sERROR;
        }
    }
#else
    return sOK_NOFEEDBACKLINE;
#endif
}

String FeederClass::reportFeederErrorState() {
    switch(this->getFeederErrorState()) {
        case sOK_NOFEEDBACKLINE:
            return "getFeederErrorState: sOK_NOFEEDBACKLINE (no feedback line for feeder, implying feeder OK)";
        case sOK:
            return "getFeederErrorState: sOK (feedbackline checked, explicit feeder OK)";
        case sERROR_IGNORED:
            return "getFeederErrorState: sERROR_IGNORED (error, but ignored per feeder setting X1)";
        case sERROR:
            return "getFeederErrorState: sERROR (error signaled on feedbackline)";
        default:
            return "illegal state in reportFeederErrorState";
    }
}

void FeederClass::enable() {
    this->feederState = sIDLE;
}

void FeederClass::disable() {
    this->feederState = sFEEDER_DISABLED;
    this->detachServo();
}

void FeederClass::update() {
#ifdef HAS_FEEDBACKLINES
    // 手动喂料检测逻辑
    if(this->feederState == sIDLE && this->hasFeedbackLine()) {
        if (millis() - this->lastTimeFeedbacklineCheck >= 10UL) {
            this->lastTimeFeedbacklineCheck = millis();
            
            int buttonState = digitalRead(feederFeedbackPinMap[this->feederNo]);
            
            if (this->feedbackLineTickCounter > 0) {
                this->feedbackLineTickCounter++;
            }
            
            if ((buttonState != this->lastButtonState) && (buttonState == LOW)) {
                this->lastButtonState = buttonState;
                this->feedbackLineTickCounter = 1;
                #ifdef DEBUG
                Serial.println(F("buttonState changed to low"));
                #endif
            } else if (buttonState != this->lastButtonState) {
                this->lastButtonState = buttonState;
            }
            
            if ((this->feedbackLineTickCounter > 5)) {
                if(buttonState == HIGH) {
                    #ifdef DEBUG
                    Serial.print(F("Manual feed triggered for feeder N"));
                    Serial.print(this->feederNo);
                    Serial.print(F(", advancing feeders default length "));
                    Serial.print(this->feederSettings.feed_length);
                    Serial.println(F("mm."));
                    #endif
                    
                    this->advance(this->feederSettings.feed_length, true);
                    this->feedbackLineTickCounter = 0;
                }
                
                if (this->feedbackLineTickCounter > 50) {
                    #ifdef DEBUG
                    Serial.println(F("Potential manual feed rejected (button pressed too long)"));
                    #endif
                    this->feedbackLineTickCounter = 0;
                }
            }
        }
    } else if (this->hasFeedbackLine()) {
        this->lastButtonState = digitalRead(feederFeedbackPinMap[this->feederNo]);
        this->feedbackLineTickCounter = 0;
    }
#endif

    // 状态机更新
    if(this->lastFeederPosition != this->feederPosition) {
        this->lastTimePositionChange = millis();
        this->lastFeederPosition = this->feederPosition;
    }

    // 时间检查
    if (millis() - this->lastTimePositionChange >= (unsigned long)this->feederSettings.time_to_settle) {
        
        // 舵机已稳定
        if(this->feederState == sADVANCING_CYCLE_COMPLETED) {
            Serial.println("ok, advancing cycle completed");
            this->detachServo();
            this->feederState = sIDLE;
        }

        // 如果无需喂料则快速退出
        if(this->remainingFeedLength == 0) {
            if(this->feederState != sFEEDER_DISABLED) {
                this->feederState = sIDLE;
            }
            return;
        } else {
            this->feederState = sMOVING;
        }
        
        #ifdef DEBUG
        Serial.print("remainingFeedLength before working: ");
        Serial.println(this->remainingFeedLength);
        #endif
        
        switch (this->feederPosition) {
            case sAT_RETRACT_POSITION: {
                if(this->remainingFeedLength >= FEEDER_MECHANICAL_ADVANCE_LENGTH) {
                    this->gotoFullAdvancedPosition();
                    this->remainingFeedLength -= FEEDER_MECHANICAL_ADVANCE_LENGTH;
                } else if(this->remainingFeedLength >= FEEDER_MECHANICAL_ADVANCE_LENGTH/2) {
                    this->gotoHalfAdvancedPosition();
                    this->remainingFeedLength -= FEEDER_MECHANICAL_ADVANCE_LENGTH/2;
                }
                break;
            }

            case sAT_HALF_ADVANCED_POSITION: {
                if(this->remainingFeedLength >= FEEDER_MECHANICAL_ADVANCE_LENGTH/2) {
                    this->gotoFullAdvancedPosition();
                    this->remainingFeedLength -= FEEDER_MECHANICAL_ADVANCE_LENGTH/2;
                }
                break;
            }

            case sAT_FULL_ADVANCED_POSITION: {
                this->gotoRetractPosition();
                break;
            }

            default: {
                break;
            }
        }

        #ifdef DEBUG
        Serial.print("remainingFeedLength after working: ");
        Serial.println(this->remainingFeedLength);
        #endif

        // 喂料完成检查
        if(this->remainingFeedLength == 0) {
            this->feederState = sADVANCING_CYCLE_COMPLETED;
        }
    }
}
