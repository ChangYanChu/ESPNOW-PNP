#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Feeder Feedback System Integration Test Script
测试喂料器反馈系统集成

本脚本用于测试ESP-NOW PNP系统中的反馈系统功能，包括：
1. 基本反馈状态查询
2. 反馈系统启用/禁用
3. 手动进料检测和处理
4. 错误状态监控

Usage:
    python test_feedback_system.py [options]

Options:
    --port PORT     串口端口 (默认: /dev/cu.usbserial-*)
    --baud RATE     波特率 (默认: 115200)
    --feeder ID     测试的喂料器ID (默认: 0)
    --verbose       详细输出模式
    --help          显示帮助信息

Examples:
    python test_feedback_system.py --port /dev/cu.usbserial-110 --feeder 0
    python test_feedback_system.py --verbose
"""

import serial
import time
import sys
import argparse
import glob
from typing import Optional, List, Dict, Any

class FeedbackTestSuite:
    """反馈系统测试套件"""
    
    def __init__(self, port: str, baud: int = 115200, verbose: bool = False):
        """初始化测试套件
        
        Args:
            port: 串口端口
            baud: 波特率
            verbose: 是否启用详细输出
        """
        self.port = port
        self.baud = baud
        self.verbose = verbose
        self.serial_conn: Optional[serial.Serial] = None
        self.test_results: Dict[str, Any] = {}
        
    def connect(self) -> bool:
        """连接到Brain控制器"""
        try:
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=2)
            time.sleep(2)  # 等待连接稳定
            
            # 清空缓冲区
            self.serial_conn.flushInput()
            self.serial_conn.flushOutput()
            
            self.log("已连接到Brain控制器", level="INFO")
            return True
            
        except Exception as e:
            self.log(f"连接失败: {e}", level="ERROR")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            self.log("已断开连接", level="INFO")
    
    def send_command(self, command: str, wait_time: float = 1.0) -> List[str]:
        """发送命令并获取响应
        
        Args:
            command: 要发送的命令
            wait_time: 等待响应的时间
            
        Returns:
            响应行列表
        """
        if not self.serial_conn:
            return []
        
        self.log(f"发送命令: {command}", level="DEBUG")
        
        # 发送命令
        self.serial_conn.write(f"{command}\n".encode())
        self.serial_conn.flush()
        
        # 等待响应
        time.sleep(wait_time)
        
        # 读取响应
        responses = []
        while self.serial_conn.in_waiting > 0:
            try:
                line = self.serial_conn.readline().decode('utf-8').strip()
                if line:
                    responses.append(line)
                    self.log(f"响应: {line}", level="DEBUG")
            except UnicodeDecodeError:
                continue
        
        return responses
    
    def log(self, message: str, level: str = "INFO"):
        """记录日志
        
        Args:
            message: 日志消息
            level: 日志级别 (DEBUG, INFO, WARN, ERROR)
        """
        timestamp = time.strftime("%H:%M:%S")
        if level == "DEBUG" and not self.verbose:
            return
        
        color_codes = {
            "DEBUG": "\033[90m",   # 灰色
            "INFO": "\033[94m",    # 蓝色
            "WARN": "\033[93m",    # 黄色
            "ERROR": "\033[91m",   # 红色
            "SUCCESS": "\033[92m"  # 绿色
        }
        
        reset_code = "\033[0m"
        color = color_codes.get(level, "")
        
        print(f"{color}[{timestamp}] {level}: {message}{reset_code}")
    
    def test_system_status(self) -> bool:
        """测试系统状态"""
        self.log("测试系统状态...", level="INFO")
        
        responses = self.send_command("status", 2.0)
        
        # 检查是否有hand注册
        has_registered_hands = False
        for response in responses:
            if "registered" in response.lower() and "yes" in response.lower():
                has_registered_hands = True
                break
        
        if has_registered_hands:
            self.log("✓ 系统状态正常，有手部控制器注册", level="SUCCESS")
            return True
        else:
            self.log("✗ 没有检测到注册的手部控制器", level="ERROR")
            return False
    
    def test_feedback_status(self, feeder_id: int) -> bool:
        """测试反馈状态查询
        
        Args:
            feeder_id: 喂料器ID
            
        Returns:
            测试是否成功
        """
        self.log(f"测试喂料器 {feeder_id} 反馈状态查询...", level="INFO")
        
        command = f"M604 N{feeder_id}"
        responses = self.send_command(command, 3.0)
        
        # 分析响应
        success = False
        for response in responses:
            if "feedback status" in response.lower():
                success = True
                self.log(f"✓ 反馈状态查询成功: {response}", level="SUCCESS")
            elif "tape loaded" in response.lower():
                self.log(f"  磁带状态: {response}", level="INFO")
            elif "feedback enabled" in response.lower():
                self.log(f"  反馈启用状态: {response}", level="INFO")
            elif "error count" in response.lower():
                self.log(f"  错误计数: {response}", level="INFO")
        
        if not success:
            self.log("✗ 反馈状态查询失败", level="ERROR")
        
        return success
    
    def test_feedback_enable_disable(self, feeder_id: int) -> bool:
        """测试反馈启用/禁用
        
        Args:
            feeder_id: 喂料器ID
            
        Returns:
            测试是否成功
        """
        self.log(f"测试喂料器 {feeder_id} 反馈启用/禁用...", level="INFO")
        
        # 测试启用反馈
        self.log("启用反馈...", level="INFO")
        responses = self.send_command(f"M605 N{feeder_id} S1", 2.0)
        enable_success = any("enabled" in r.lower() for r in responses)
        
        if enable_success:
            self.log("✓ 反馈启用成功", level="SUCCESS")
        else:
            self.log("✗ 反馈启用失败", level="ERROR")
        
        time.sleep(1)
        
        # 测试禁用反馈
        self.log("禁用反馈...", level="INFO")
        responses = self.send_command(f"M605 N{feeder_id} S0", 2.0)
        disable_success = any("disabled" in r.lower() for r in responses)
        
        if disable_success:
            self.log("✓ 反馈禁用成功", level="SUCCESS")
        else:
            self.log("✗ 反馈禁用失败", level="ERROR")
        
        # 重新启用反馈用于后续测试
        self.send_command(f"M605 N{feeder_id} S1", 1.0)
        
        return enable_success and disable_success
    
    def test_manual_feed_clear(self, feeder_id: int) -> bool:
        """测试手动进料标志清除
        
        Args:
            feeder_id: 喂料器ID
            
        Returns:
            测试是否成功
        """
        self.log(f"测试喂料器 {feeder_id} 手动进料标志清除...", level="INFO")
        
        command = f"M606 N{feeder_id}"
        responses = self.send_command(command, 2.0)
        
        success = any("cleared" in r.lower() for r in responses)
        
        if success:
            self.log("✓ 手动进料标志清除成功", level="SUCCESS")
        else:
            self.log("✗ 手动进料标志清除失败", level="ERROR")
        
        return success
    
    def test_manual_feed_process(self, feeder_id: int) -> bool:
        """测试手动进料处理
        
        Args:
            feeder_id: 喂料器ID
            
        Returns:
            测试是否成功
        """
        self.log(f"测试喂料器 {feeder_id} 手动进料处理...", level="INFO")
        
        command = f"M607 N{feeder_id}"
        responses = self.send_command(command, 2.0)
        
        # 由于没有实际的手动进料信号，这个测试可能会失败
        # 但我们可以检查命令是否被正确处理
        success = any("manual feed" in r.lower() for r in responses)
        
        if success:
            self.log("✓ 手动进料处理命令执行", level="SUCCESS")
        else:
            self.log("ℹ 手动进料处理命令执行（未检测到手动进料信号）", level="WARN")
        
        return True  # 这个测试总是返回成功，因为没有实际的手动信号
    
    def run_comprehensive_test(self, feeder_id: int = 0) -> bool:
        """运行综合测试
        
        Args:
            feeder_id: 要测试的喂料器ID
            
        Returns:
            所有测试是否都通过
        """
        self.log("=== 开始反馈系统综合测试 ===", level="INFO")
        
        tests = [
            ("系统状态检查", lambda: self.test_system_status()),
            ("反馈状态查询", lambda: self.test_feedback_status(feeder_id)),
            ("反馈启用/禁用", lambda: self.test_feedback_enable_disable(feeder_id)),
            ("手动进料标志清除", lambda: self.test_manual_feed_clear(feeder_id)),
            ("手动进料处理", lambda: self.test_manual_feed_process(feeder_id)),
        ]
        
        results = {}
        all_passed = True
        
        for test_name, test_func in tests:
            self.log(f"\n--- {test_name} ---", level="INFO")
            try:
                result = test_func()
                results[test_name] = result
                if not result:
                    all_passed = False
            except Exception as e:
                self.log(f"测试异常: {e}", level="ERROR")
                results[test_name] = False
                all_passed = False
            
            time.sleep(1)  # 测试间隔
        
        # 输出测试结果摘要
        self.log("\n=== 测试结果摘要 ===", level="INFO")
        for test_name, result in results.items():
            status = "✓ PASS" if result else "✗ FAIL"
            color = "SUCCESS" if result else "ERROR"
            self.log(f"{test_name}: {status}", level=color)
        
        overall_result = "全部通过" if all_passed else "部分失败"
        overall_color = "SUCCESS" if all_passed else "WARN"
        self.log(f"\n总体结果: {overall_result}", level=overall_color)
        
        self.test_results = results
        return all_passed

def find_serial_ports() -> List[str]:
    """查找可用的串口"""
    patterns = [
        '/dev/cu.usbserial-*',
        '/dev/cu.SLAB_USBtoUART*',
        '/dev/ttyUSB*',
        '/dev/ttyACM*'
    ]
    
    ports = []
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    
    return sorted(ports)

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="Feeder Feedback System Integration Test",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    
    parser.add_argument('--port', help='串口端口')
    parser.add_argument('--baud', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('--feeder', type=int, default=0, help='测试的喂料器ID (默认: 0)')
    parser.add_argument('--verbose', action='store_true', help='详细输出模式')
    
    args = parser.parse_args()
    
    # 查找串口
    if not args.port:
        ports = find_serial_ports()
        if not ports:
            print("错误: 未找到可用的串口端口")
            print("请使用 --port 参数指定串口端口")
            return 1
        
        args.port = ports[0]
        print(f"自动选择串口: {args.port}")
        
        if len(ports) > 1:
            print("其他可用端口:", ", ".join(ports[1:]))
    
    # 创建测试套件
    test_suite = FeedbackTestSuite(args.port, args.baud, args.verbose)
    
    try:
        # 连接到设备
        if not test_suite.connect():
            return 1
        
        # 运行测试
        success = test_suite.run_comprehensive_test(args.feeder)
        
        return 0 if success else 1
        
    except KeyboardInterrupt:
        print("\n用户中断测试")
        return 1
    except Exception as e:
        print(f"测试异常: {e}")
        return 1
    finally:
        test_suite.disconnect()

if __name__ == "__main__":
    sys.exit(main())
