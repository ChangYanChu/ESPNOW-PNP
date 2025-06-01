import serial
import time
import sys

class FeederTester:
    def __init__(self, port, baudrate=115200):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=2)
            time.sleep(2)  # 等待ESP32C3启动
            print(f"Connected to {port} at {baudrate} baud")
        except Exception as e:
            print(f"Error connecting to {port}: {e}")
            sys.exit(1)
    
    def send_command(self, command):
        """发送G-code命令并获取响应"""
        print(f"Sending: {command}")
        self.ser.write((command + '\n').encode())
        time.sleep(0.1)
        
        response = ""
        while self.ser.in_waiting:
            response += self.ser.read(self.ser.in_waiting).decode()
            time.sleep(0.1)
        
        print(f"Response: {response.strip()}")
        return response
    
    def test_basic_commands(self):
        """测试基本命令"""
        print("\n=== Testing Basic Commands ===")
        
        # 启用喂料器
        response = self.send_command("M610 S1")
        assert "ok" in response, "Failed to enable feeders"
        
        # 查询状态
        response = self.send_command("M610")
        assert "powerState: 1" in response, "Feeder state incorrect"
        
        # 检查喂料器状态
        response = self.send_command("M602 N0")
        assert "ok" in response, "Feeder status check failed"
        
        print("✓ Basic commands test passed")
    
    def test_servo_control(self):
        """测试舵机控制"""
        print("\n=== Testing Servo Control ===")
        
        angles = [0, 40, 80, 0]
        for angle in angles:
            response = self.send_command(f"M280 N0 A{angle}")
            assert "ok" in response, f"Failed to set angle {angle}"
            time.sleep(1)  # 等待舵机移动
        
        print("✓ Servo control test passed")
    
    def test_feeding_sequence(self):
        """测试喂料序列"""
        print("\n=== Testing Feeding Sequence ===")
        
        # 测试不同喂料长度
        feed_lengths = [2, 4, 6, 8]
        for length in feed_lengths:
            response = self.send_command(f"M600 N0 F{length}")
            assert "ok" in response, f"Failed to feed {length}mm"
            
            # 等待喂料完成
            time.sleep(3)
            
            # 取料后回缩
            response = self.send_command("M601 N0")
            assert "ok" in response, "Failed post-pick retract"
            
            time.sleep(1)
        
        print("✓ Feeding sequence test passed")
    
    def test_error_handling(self):
        """测试错误处理"""
        print("\n=== Testing Error Handling ===")
        
        # 禁用喂料器后尝试操作
        self.send_command("M610 S0")
        response = self.send_command("M600 N0 F4")
        assert "error" in response, "Should return error when disabled"
        
        # 重新启用
        self.send_command("M610 S1")
        
        # 无效喂料器编号
        response = self.send_command("M600 N99 F4")
        assert "error" in response, "Should return error for invalid feeder"
        
        # 无效喂料长度
        response = self.send_command("M600 N0 F3")
        assert "error" in response, "Should return error for invalid feed length"
        
        # 无效角度
        response = self.send_command("M280 N0 A200")
        assert "error" in response, "Should return error for invalid angle"
        
        print("✓ Error handling test passed")
    
    def test_multiple_feeders(self):
        """测试多个喂料器"""
        print("\n=== Testing Multiple Feeders ===")
        
        for feeder_id in range(min(4, 8)):  # 测试前4个喂料器
            print(f"Testing Feeder {feeder_id}")
            
            # 检查状态
            response = self.send_command(f"M602 N{feeder_id}")
            assert "ok" in response, f"Feeder {feeder_id} status check failed"
            
            # 角度测试
            response = self.send_command(f"M280 N{feeder_id} A80")
            assert "ok" in response, f"Feeder {feeder_id} angle set failed"
            time.sleep(0.5)
            
            response = self.send_command(f"M280 N{feeder_id} A0")
            assert "ok" in response, f"Feeder {feeder_id} return to 0 failed"
            time.sleep(0.5)
        
        print("✓ Multiple feeders test passed")
    
    def run_all_tests(self):
        """运行所有测试"""
        try:
            self.test_basic_commands()
            self.test_servo_control()
            self.test_feeding_sequence()
            self.test_error_handling()
            self.test_multiple_feeders()
            
            print("\n🎉 All tests passed!")
            
        except AssertionError as e:
            print(f"\n❌ Test failed: {e}")
        except Exception as e:
            print(f"\n💥 Unexpected error: {e}")
    
    def close(self):
        self.ser.close()

if __name__ == "__main__":
    # 根据你的系统修改串口
    # Windows: "COM3", "COM4" 等
    # macOS: "/dev/tty.usbserial-*" 或 "/dev/cu.usbserial-*"
    # Linux: "/dev/ttyUSB0", "/dev/ttyACM0" 等
    
    port = "/dev/tty.usbmodem101"  # 修改为你的串口
    
    tester = FeederTester(port)
    tester.run_all_tests()
    tester.close()
