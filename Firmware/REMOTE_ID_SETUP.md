# Hand远程ID配置使用说明

## 功能概述
现在支持统一编译Hand固件，并通过Brain端远程设置Hand的Feeder ID，无需逐个烧录不同的固件。

## 使用流程

### 1. 编译和烧录
- Hand固件：统一编译，默认ID为255（未分配状态，指EEPROM中存储的ID为255）
- Brain固件：正常编译

### 2. Hand上电后的行为
- 如果EEPROM中存储的ID是255（未分配），Hand会进入配置模式
- 配置模式下，Hand会定期发送发现请求，等待Brain分配ID
- 已分配ID的Hand正常工作

### 3. Brain端配置命令

#### 查看未分配的Hand
```
M630
```
返回示例：
```
ok Unassigned Hands:
0: AA:BB:CC:DD:EE:FF
1: 11:22:33:44:55:66
```

#### 为Hand分配ID
```
M631 N<hand_index> P<new_feeder_id>
```
- N: Hand在未分配列表中的索引（从M630命令得到）
- P: 要分配的Feeder ID（0-49）

示例：为列表中第0个Hand分配ID为5
```
M631 N0 P5
```

### 4. 配置完成
- Hand收到ID配置命令后会：
  1. 验证新ID的有效性
  2. 保存到EEPROM
  3. 发送确认响应
  4. 自动重启
  5. 重启后使用新ID正常工作

## 注意事项
1. 每个Hand只需要配置一次，EEPROM会保存设置
2. 如需重新配置，可通过串口发送 `SET_ID 255` 将Hand重置为未分配状态
3. 未分配的Hand会在Brain的未分配列表中保持30秒，超时后需要重新上电

## 调试
- Hand端：通过修改 `DEBUG_MODE` 为1可启用串口调试
- Brain端：通过串口或TCP连接可查看调试信息
