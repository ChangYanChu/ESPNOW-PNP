#ifndef HAND_BUTTON_H
#define HAND_BUTTON_H

#include <OneButton.h>

// 按钮事件回调函数类型
typedef void (*ButtonCallback)();

// 按钮控制函数
void initButton();
void handleButton();
void setButtonDoubleClickCallback(ButtonCallback callback);
void setButtonSingleClickCallback(ButtonCallback callback);
void setButtonLongPressCallback(ButtonCallback callback);

#endif