import matplotlib.pyplot as plt

def plot_cwnd():
    times = []
    cwnds = []
    
    log_file = "cwnd_log.txt"
    
    try:
        with open(log_file, 'r') as f:
            lines = f.readlines()
            
            # 跳过第一行表头
            for line in lines[1:]:
                if not line.strip():
                    continue
                parts = line.strip().split()
                if len(parts) >= 2:
                    times.append(float(parts[0]))
                    cwnds.append(float(parts[1]))
                    
        if not times:
            print(f"Error: {log_file} is empty or invalid.")
            return

        plt.figure(figsize=(10, 6))
        plt.plot(times, cwnds, label='CWND Size', color='b', linewidth=1.5)
        
        plt.title('Reno Congestion Control Analysis (CWND vs Time)')
        plt.xlabel('Time (ms)')
        plt.ylabel('Congestion Window Size (packets)')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend()
        
        output_file = 'cwnd_plot_4jpg.png'
        plt.savefig(output_file, dpi=300)
        print(f"Plot saved successfully as '{output_file}'")
        plt.show()

    except FileNotFoundError:
        print(f"Error: Could not find '{log_file}'. Please run sender.exe first to generate data.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    plot_cwnd()
