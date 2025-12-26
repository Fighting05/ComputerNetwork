import pyshark
import os
import sys
import datetime

# 配置
pcap_file = os.path.join(os.path.dirname(__file__), 'lab3_capture.pcapng')
output_file = os.path.join(os.path.dirname(__file__), 'pcap_detailed_stats.txt')
target_port = '1234'

class Logger(object):
    def __init__(self):
        self.terminal = sys.stdout
        self.log = open(output_file, "w", encoding='utf-8')

    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)

    def flush(self):
        self.terminal.flush()
        self.log.flush()

sys.stdout = Logger()

def print_header(title):
    print(f"\n{'='*50}\n{title}\n{'='*50}")

def analyze_tcp_options(pkt):
    """提取 TCP 高级选项 (MSS, Window Scale)"""
    options = []
    if hasattr(pkt.tcp, 'options_mss_val'):
        options.append(f"MSS={pkt.tcp.options_mss_val}")
    if hasattr(pkt.tcp, 'options_wscale_val'):
        options.append(f"WScale={pkt.tcp.options_wscale_val}")
    if hasattr(pkt.tcp, 'options_sack_perm'):
        options.append("SACK_Perm")
    return ", ".join(options) if options else "None"

def get_flag_safe(val):
    if str(val) == 'True': return 1
    if str(val) == 'False': return 0
    try: return int(val, 16)
    except: return 0

def main():
    # ... (前面的代码不变) ...

    # 2. 详细的握手参数分析
    print_header("TCP HANDSHAKE DETAILED PARAMETERS (握手参数详情)")
    cap = pyshark.FileCapture(pcap_file, display_filter=f'tcp.port == {target_port}', tshark_path=tshark_path)
    count = 0
    for pkt in cap:
        if 'TCP' in pkt:
            # 使用更安全的属性访问方式
            try:
                flags_val = pkt.tcp.flags
                if str(flags_val).startswith('0x'):
                    flags = int(flags_val, 16)
                else:
                    # 如果 flags 直接返回了预解析的对象，尝试获取原始值
                    # 这里简化处理：直接看 syn 标志位
                    flags = 0 
            except:
                flags = 0
            
            # 直接检查 syn 标志位
            is_syn = get_flag_safe(pkt.tcp.flags_syn)
            is_ack = get_flag_safe(pkt.tcp.flags_ack)

            if is_syn:
                count += 1
                role = "Client -> Server" if not is_ack else "Server -> Client"
                print(f"[{role}]")
                print(f"  Seq: {pkt.tcp.seq}")
                print(f"  Window Size: {pkt.tcp.window_size_value}")
                print(f"  Advanced Options: {analyze_tcp_options(pkt)}")
            if count >= 2: break
    cap.close()

    # 3. HTTP 性能分析
    print_header("HTTP PERFORMANCE (HTTP 性能分析)")
    cap = pyshark.FileCapture(pcap_file, display_filter=f'tcp.port == {target_port} && http', tshark_path=tshark_path)
    req_time = 0
    
    for pkt in cap:
        if hasattr(pkt.http, 'request_method'):
            req_time = float(pkt.sniff_timestamp)
            print(f"Request: {pkt.http.request_method} {pkt.http.request_uri}")
        elif hasattr(pkt.http, 'response_code'):
            res_time = float(pkt.sniff_timestamp)
            if req_time > 0:
                print(f"Response: {pkt.http.response_code} (Processing Time: {(res_time - req_time)*1000:.2f} ms)")
    cap.close()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error: {e}")