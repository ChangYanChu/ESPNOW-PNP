# ESP-NOW PNP 异步响应流程修改总结

## 修改目标
将原来的同步响应流程改为异步响应流程，确保Brain在收到Hand的完成确认后再返回G-code OK响应。

## 原始流程
1. Brain收到 `MCODE_ADVANCE` (M600) 指令
2. Brain通过ESP-NOW发送到Hand
3. **Brain立即调用 `sendAnswer(0, F("Feeder advance command sent"))` 返回**
4. Hand处理喂料推进

## 修改后流程
1. Brain收到 `MCODE_ADVANCE` (M600) 指令
2. Brain通过ESP-NOW发送到Hand
3. **Brain等待Hand完成**
4. Hand处理喂料推进
5. **Hand发送 `CMD_RESPONSE` 给Brain**
6. **Brain收到 `CMD_RESPONSE` 后调用 `sendAnswer(0, F("Feeder advance command completed"))` 返回**

## 具体修改

### 1. Brain端修改 (`src/brain/brain_espnow.cpp`)

#### 1.1 修改 `dataReceived` 函数
- 添加对 `ESPNowResponse` 数据包的处理
- 当收到 `CMD_RESPONSE` 类型的响应时：
  - 如果状态为 `STATUS_OK`：调用 `sendAnswer(0, F("Feeder advance command completed"))`
  - 如果状态为错误：调用 `sendAnswer(1, "Feeder error: " + message)`

#### 1.2 修改 `sendFeederAdvanceCommand` 函数
- 移除ESP-NOW发送成功后立即调用 `sendAnswer` 的代码
- ESP-NOW发送成功时只打印日志，不回复G-code
- ESP-NOW发送失败时仍然立即返回错误

### 2. Brain端修改 (`src/brain/gcode.cpp`)

#### 2.1 修改 `MCODE_ADVANCE` 处理逻辑
- 更新注释说明新的流程
- 移除不准确的错误消息

### 3. Hand端修改 (`src/hand/hand_espnow.cpp`)

#### 3.1 修改 `handleFeederAdvanceCommand` 函数
- 启用原来被注释的 `sendSuccessResponse(feederID, "Feed OK")` 调用
- 确保喂料完成后发送响应给Brain

## 流程验证

### 成功情况
1. Brain发送ESP-NOW命令 → Hand收到命令
2. Hand执行 `feedTapeAction(feedLength)` 
3. Hand调用 `sendSuccessResponse(feederID, "Feed OK")`
4. Brain的 `dataReceived` 收到 `CMD_RESPONSE`
5. Brain调用 `sendAnswer(0, F("Feeder advance command completed"))`
6. G-code客户端收到 `ok Feeder advance command completed`

### 失败情况
1. ESP-NOW发送失败 → Brain立即返回 `error Failed to send feeder advance command`
2. Hand执行失败 → Hand可调用 `sendErrorResponse()` → Brain返回错误信息

## 注意事项

1. **超时处理**：当前实现没有添加超时机制，如果Hand没有响应，Brain将一直等待
2. **错误处理**：Hand需要在遇到错误时调用 `sendErrorResponse()` 而不是 `sendSuccessResponse()`
3. **并发处理**：当前实现假设一次只有一个推进命令，多个并发命令可能需要额外的序列号管理

## 后续改进建议

1. 添加超时机制，避免无限等待
2. 添加命令序列号，支持多个并发命令
3. 增强错误处理和重试机制
4. 添加命令状态跟踪和管理
