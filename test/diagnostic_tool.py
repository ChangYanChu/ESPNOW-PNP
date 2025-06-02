#!/usr/bin/env python3
"""
ESP-NOW PNP 系统快速诊断脚本
用于验证反馈系统和通信状态
"""

import serial
import time
import sys

def find_serial_port():
    """自动查找串口"""
    import serial.tools.list_ports
    
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'USB' in port.description or 'Serial' in port.description:
            return port.device
    
    # 常见的串口设备
    common_ports = ['/dev/ttyUSB0', '/dev/ttyACM0', 'COM3', 'COM4', 'COM5']
    for port in common_ports:
        try:
            ser = serial.Serial(port, timeout=1)
            ser.close()
            return port
        except:
            continue
    
    return None

def send_command(ser, command, timeout=2):
    """发送G-code命令并获取响应"""
    print(f"→ {command}")
    ser.write(f"{command}\n".encode())
    ser.flush()
    
    response_lines = []
    start_time = time.time()
    
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"← {line}")
                response_lines.append(line)
                
                # 检查是否收到完整响应
                if line.startswith('ok') or line.startswith('error'):
                    break
    
    return response_lines

def test_system_status(ser):
    """测试系统基本状态"""
    print("\n" + "="*50)
    print("1. 系统状态检查")
    print("="*50)
    
    # 检查系统启用状态
    send_command(ser, "M610")
    time.sleep(0.5)
    
    # 检查Hand连接状态
    send_command(ser, "M620")
    time.sleep(1)
    
    return True

def test_feedback_system(ser, feeder_id=0):
    """测试反馈系统"""
    print("\n" + "="*50)
    print(f"2. 反馈系统测试 (喂料器 {feeder_id})")
    print("="*50)
    
    # 查询反馈状态
    send_command(ser, f"M604 N{feeder_id}")
    time.sleep(0.5)
    
    # 查询喂料器状态
    send_command(ser, f"M602 N{feeder_id}")
    time.sleep(0.5)
    
    return True

def test_feedback_control(ser, feeder_id=0):
    """测试反馈控制功能"""
    print("\n" + "="*50)
    print(f"3. 反馈控制测试 (喂料器 {feeder_id})")
    print("="*50)
    
    # 禁用反馈
    print("禁用反馈...")
    send_command(ser, f"M605 N{feeder_id} S0")
    time.sleep(0.5)
    
    # 检查状态
    send_command(ser, f"M604 N{feeder_id}")
    time.sleep(0.5)
    
    # 启用反馈
    print("启用反馈...")
    send_command(ser, f"M605 N{feeder_id} S1")
    time.sleep(0.5)
    
    # 再次检查状态
    send_command(ser, f"M604 N{feeder_id}")
    time.sleep(0.5)
    
    return True

def test_communication_sync(ser):
    """测试通信同步"""
    print("\n" + "="*50)
    print("4. 通信同步测试")
    print("="*50)
    
    print("清除注册...")
    send_command(ser, "clear_registration")
    time.sleep(1)
    
    print("重新发现Hand...")
    send_command(ser, "discovery")
    time.sleep(3)
    
    print("检查连接状态...")
    send_command(ser, "status")
    time.sleep(1)
    
    return True

def test_manual_feed(ser, feeder_id=0):
    """测试手动进料功能"""
    print("\n" + "="*50)
    print(f"5. 手动进料测试 (喂料器 {feeder_id})")
    print("="*50)
    
    print("请手动短按微动开关 (10-30ms)...")
    print("等待10秒检测手动进料...")
    
    for i in range(10):
        print(f"检测中... {i+1}/10")
        send_command(ser, f"M604 N{feeder_id}", timeout=1)
        
        # 检查是否有手动进料
        response = send_command(ser, f"M607 N{feeder_id}", timeout=1)
        
        time.sleep(1)
    
    # 清除手动进料标志
    print("清除手动进料标志...")
    send_command(ser, f"M606 N{feeder_id}")
    
    return True

def test_feeding_operation(ser, feeder_id=0):
    """测试推进操作"""
    print("\n" + "="*50)
    print(f"6. 推进操作测试 (喂料器 {feeder_id})")
    print("="*50)
    
    # 启用系统
    print("启用系统...")
    send_command(ser, "M610 S1")
    time.sleep(0.5)
    
    # 正常推进测试
    print("测试正常推进...")
    send_command(ser, f"M600 N{feeder_id} F4")
    time.sleep(1)
    
    # 强制推进测试
    print("测试强制推进 (忽略反馈)...")
    send_command(ser, f"M600 N{feeder_id} F4 X1")
    time.sleep(1)
    
    # 回缩测试
    print("测试回缩...")
    send_command(ser, f"M601 N{feeder_id}")
    time.sleep(1)
    
    return True

def interactive_mode(ser):
    """交互模式"""
    print("\n" + "="*50)
    print("7. 交互模式")
    print("="*50)
    print("输入G-code命令进行手动测试")
    print("输入 'help' 查看常用命令")
    print("输入 'quit' 或 'exit' 退出")
    
    while True:
        try:
            command = input("\nGCode> ").strip()
            
            if command.lower() in ['quit', 'exit', 'q']:
                break
            elif command.lower() == 'help':
                print_help()
            elif command:
                send_command(ser, command)
        except KeyboardInterrupt:
            print("\n退出交互模式...")
            break

def print_help():
    """打印帮助信息"""
    print("""
常用G-code命令：
  M610        - 查询系统状态
  M610 S1     - 启用系统
  M610 S0     - 禁用系统
  M620        - 查询所有Hand状态
  M602 N0     - 查询喂料器0状态
  M604 N0     - 检查喂料器0反馈状态
  M605 N0 S1  - 启用喂料器0反馈
  M605 N0 S0  - 禁用喂料器0反馈
  M600 N0 F4  - 推进喂料器0 (4mm)
  M600 N0 F4 X1 - 强制推进喂料器0
  M601 N0     - 回缩喂料器0
  M606 N0     - 清除喂料器0手动进料标志
  M607 N0     - 处理喂料器0手动进料

管理命令：
  discovery           - 重新发现Hand
  clear_registration  - 清除注册
  status             - 显示连接状态
    """)

def main():
    print("ESP-NOW PNP 系统快速诊断脚本")
    print("="*50)
    
    # 查找串口
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = find_serial_port()
        if not port:
            print("错误: 无法找到串口设备")
            print("请手动指定串口: python3 diagnostic_tool.py /dev/ttyUSB0")
            return
    
    print(f"使用串口: {port}")
    
    try:
        # 连接串口
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(2)  # 等待连接稳定
        
        print(f"成功连接到 {port}")
        
        # 选择测试模式
        print("\n选择测试模式:")
        print("1. 完整诊断测试")
        print("2. 快速状态检查")
        print("3. 反馈系统测试")
        print("4. 通信同步测试")
        print("5. 交互模式")
        
        choice = input("\n请选择 (1-5): ").strip()
        
        if choice == "1":
            # 完整诊断
            test_system_status(ser)
            test_feedback_system(ser)
            test_feedback_control(ser)
            test_communication_sync(ser)
            test_manual_feed(ser)
            test_feeding_operation(ser)
            interactive_mode(ser)
            
        elif choice == "2":
            # 快速检查
            test_system_status(ser)
            test_feedback_system(ser)
            
        elif choice == "3":
            # 反馈测试
            test_feedback_system(ser)
            test_feedback_control(ser)
            test_manual_feed(ser)
            
        elif choice == "4":
            # 通信测试
            test_communication_sync(ser)
            test_system_status(ser)
            
        elif choice == "5":
            # 交互模式
            interactive_mode(ser)
            
        else:
            print("无效选择，启动交互模式...")
            interactive_mode(ser)
        
        print("\n诊断完成!")
        
    except serial.SerialException as e:
        print(f"串口连接错误: {e}")
        print("请检查:")
        print("1. 设备是否正确连接")
        print("2. 串口是否被其他程序占用")
        print("3. 用户是否有串口访问权限")
        
    except KeyboardInterrupt:
        print("\n诊断被用户中断")
        
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
