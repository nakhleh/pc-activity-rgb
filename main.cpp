#include <iostream>
#include <iomanip>
#include <array>
#include <thread>

#include "ComputerActivity.h"
#include "RgbLighting.h"

using namespace std;

int main() {
	ComputerActivity* activity = new ComputerActivity();
	RgbLighting* lighting = new RgbLighting();
	auto deviceMap = lighting->get_device_mapping();
	
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
		lighting->load_device_colors_activity("cpu", 0, deviceMap["CommanderPro"], cpuPct, ledMap);
		lighting->load_device_colors_activity("gpu", 0, deviceMap["CommanderPro"], gpuPct, ledMap);

		// Assumes 4 sticks of RAM, exercise left to generalize
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"],      min(memPct*4, 100), ledMap);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 1,  min((memPct-25)*4, 100), ledMap);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 2,  min((memPct-50)*4, 100), ledMap);
	 	lighting->load_device_colors_activity("ram", 0, deviceMap["MemoryModule"] + 3,  min((memPct-75)*4, 100), ledMap);

		// Show time on fans, hour (top), first digit of minute, second digit (bottom)
		lighting->load_device_colors_binary("fan", 0, deviceMap["CommanderPro"], time->tm_hour % 12, ledMap);
		lighting->load_device_colors_binary("fan", 1, deviceMap["CommanderPro"], time->tm_min / 10, ledMap);
		lighting->load_device_colors_binary("fan", 2, deviceMap["CommanderPro"], time->tm_min % 10, ledMap);

	 	lighting->set_colors(ledMap);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	// For debugging
	//cout << "Press enter to exit";
	//cin.get();
}
