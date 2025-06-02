#!/usr/bin/env python3
"""
G-code控制测试脚本
用于测试Brain控制器的G-code处理功能
"""

import serial
import time
import sys

def find_serial_port():
    """查找可用的串口"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        if 'usbmodem' in port.device or 'ttyUSB' in port.device:
            return port.device
    
    if ports:
        print("Available ports:")
        for i, port in enumerate(ports):
            print(f"  {i}: {port.device} - {port.description}")
        
        choice = input("Select port number (0-{}): ".format(len(ports)-1))
        try:
            return ports[int(choice)].device
        except (ValueError, IndexError):
            return ports[0].device
    
    return None

def send_command(ser, command):
    """发送命令并读取响应"""
    print(f"\n>>> {command}")
    ser.write((command + '\n').encode())
    time.sleep(0.1)
    
    # 读取响应
    response = ""
    start_time = time.time()
    while time.time() - start_time < 2:  # 2秒超时
        if ser.in_waiting:
            data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            response += data
            if 'ok' in response or 'error' in response:
                break
        time.sleep(0.01)
    
    print(f"<<< {response.strip()}")
    return response

def main():
    # 查找串口
    port = find_serial_port()
    if not port:
        print("No serial port found!")
        return
    
    print(f"Using port: {port}")
    
    try:
        # 打开串口
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # 等待连接稳定
        
        print("\n=== G-code Control Test ===")
        
        # 测试序列
        test_commands = [
            # 1. 检查系统状态
            "status",
            
            # 2. 启用系统
            "M610 S1",
            
            # 3. 检查手部状态
            "M620",
            
            # 4. 测试正确的喂料器（feederId: 0）
            "M600 N0 F4",
            
            # 5. 测试错误的喂料器（feederId: 1）
            "M600 N1 F4",
            
            # 6. 测试舵机角度控制
            "M280 N0 A90",
            
            # 7. 查询喂料器状态
            "M602 N0",
            "M602 N1",
        ]
        
        for cmd in test_commands:
            send_command(ser, cmd)
            time.sleep(0.5)
        
        print("\n=== Interactive Mode ===")
        print("Type 'quit' to exit")
        
        while True:
            cmd = input("\nEnter command: ").strip()
            if cmd.lower() in ['quit', 'exit', 'q']:
                break
            if cmd:
                send_command(ser, cmd)
        
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
