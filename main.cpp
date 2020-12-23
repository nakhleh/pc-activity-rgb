#include <iostream>
#include <iomanip>
#include <array>
#include <thread>

#include "ComputerActivity.h"
#include "RgbLighting.h"

using namespace std;

// Colors
Color cpu_base, cpu_active, gpu_base, gpu_active, ram_base, ram_active, pump, fans_one, fans_zero;

void green_theme() {
	Color green{0, 255, 0};
	Color red{255, 0, 0};
	Color green_dim{0, 32, 0};
	Color red_dim{32, 0, 0};
	Color off{0, 0, 0};
	cpu_base = gpu_base = green;
	cpu_active = gpu_active = red;
	ram_base = green_dim;
	ram_active = red_dim;
	fans_one = green;
	fans_zero = off;
	pump = green;
}

void cyberpunk_theme() {
	Color yellow{245, 242, 32};
	Color blue{66, 230, 245};
	Color red{255, 0, 0};
	Color yellow_dim{60, 60, 8};
	Color red_dim{64, 0, 0};
	Color off{0, 0, 0};

	cpu_base = gpu_base = blue;
	cpu_active = gpu_active = red;
	ram_base = yellow_dim;
	ram_active = red_dim;
	pump = yellow;
	fans_one = blue;
	fans_zero = yellow;
}

int main() {
	ComputerActivity* activity = new ComputerActivity();
	RgbLighting* lighting = new RgbLighting();
	auto deviceMap = lighting->get_device_mapping();

	// Set theme
	//green_theme();
	cyberpunk_theme();

	while(true) {
		time_t curTime = time(NULL);
		tm *time = localtime(&curTime);
		auto memPct = activity->get_memory_usage();
		auto cpuPct = activity->get_cpu_load();
		auto gpuPct = activity->get_gpu_load();
		cout << "Time: " << setw(2) << setfill('0') << time->tm_hour 
		                 << ":" << setw(2) << setfill('0') << time->tm_min
						 << ":" << setw(2) << setfill('0') << time->tm_sec;
		cout << ", Memory: " << memPct << "%";
		cout << ", CPU:" << cpuPct << "%";
		cout << ", GPU:" << gpuPct << "%" << endl;

	 	LedMap ledMap = lighting->get_led_arrays();
		lighting->load_device_colors_activity("cpu", 0, deviceMap["CommanderPro"], cpuPct, ledMap, cpu_base, cpu_active);
		lighting->load_device_colors_activity("gpu", 0, deviceMap["CommanderPro"], gpuPct, ledMap, gpu_base, gpu_active);

		// Assumes 4 sticks of RAM, exercise left to generalize
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"],      min(memPct*4, 100), ledMap, ram_base, ram_active);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 1,  min((memPct-25)*4, 100), ledMap, ram_base, ram_active);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 2,  min((memPct-50)*4, 100), ledMap, ram_base, ram_active);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 3,  min((memPct-75)*4, 100), ledMap, ram_base, ram_active);

		// Show time on fans, hour (top), first digit of minute, second digit (bottom)
		lighting->load_device_colors_binary("fan", 0, deviceMap["CommanderPro"], time->tm_hour % 12, ledMap, fans_one, fans_zero);
		lighting->load_device_colors_binary("fan", 1, deviceMap["CommanderPro"], time->tm_min / 10, ledMap, fans_one, fans_zero);
		lighting->load_device_colors_binary("fan", 2, deviceMap["CommanderPro"], time->tm_min % 10, ledMap, fans_one, fans_zero);
		
		lighting->load_device_colors_static("reservoir", 0, deviceMap["CommanderPro"], ledMap, pump);

	 	lighting->set_colors(ledMap);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	// For debugging
	//cout << "Press enter to exit";
	//cin.get();
}
