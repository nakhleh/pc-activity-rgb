#include <array>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "pdh.h"
#include "windows.h"

#include "ComputerActivity.h"

using namespace std;

int main() {
	ComputerActivity* activity = new ComputerActivity();
	
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

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	// For debugging
	//cout << "Press enter to exit";
	//cin.get();
}
