#include "hand_button.h"
#include "hand_config.h"

// 按钮对象
static OneButton button;

// 回调函数指针
static ButtonCallback doubleClickCallback = nullptr;
static ButtonCallback singleClickCallback = nullptr;
static ButtonCallback longPressCallback = nullptr;

// 内部回调函数
void onButtonDoubleClick() {
    if (doubleClickCallback) {
        doubleClickCallback();
    }
}

void onButtonSingleClick() {
    if (singleClickCallback) {
        singleClickCallback();
    }
}

void onButtonLongPress() {
    if (longPressCallback) {
        longPressCallback();
    }
}

void initButton() {
    button.setup(BUTTON_PIN, INPUT_PULLUP, BUTTON_ACTIVE_LOW);
    button.attachDoubleClick(onButtonDoubleClick);
    button.attachClick(onButtonSingleClick);
    button.attachLongPressStart(onButtonLongPress);
    
    DEBUG_PRINTLN("Button initialized on GPIO3");
}

void handleButton() {
    button.tick();
}

void setButtonDoubleClickCallback(ButtonCallback callback) {
    doubleClickCallback = callback;
}

void setButtonSingleClickCallback(ButtonCallback callback) {
    singleClickCallback = callback;
}

void setButtonLongPressCallback(ButtonCallback callback) {
    longPressCallback = callback;
}