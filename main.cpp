#include <array>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

// Windows APIs for getting system data

extern "C" {
#include "nvml.h"
}
#include "pdh.h"
#include "windows.h"

// iCUE API for controlling lighting
#include <CUESDK.h>

using namespace std;

using LedMap = unordered_map<int, vector<CorsairLedColor>>;

struct DevInfoType {
	int deviceIndex;
	int ledCount;
	int ledStartIndex;
};

// Maps logical devices to physical iCUE indicies
static const unordered_map<string, vector<DevInfoType>> devInfo = {
	{ "gpu", {
				{0, 16,  0}
	}},
	{ "reservoir", {
				{0, 10, 16}
	}},
	{ "cpu", {
				{0, 16, 26}
	}},
	{ "fan", { 
				{0,  4, 42},
				{0,  4, 50},
				{0,  4, 46},
	}},
	{ "ram", {
				{1, 10, 0},
				{2, 10, 0},
				{3, 10, 0},
				{4, 10, 0}
	}}
};

static const char * DeviceTypeStrings[] = { "Unknown", "Mouse", "Keyboard", "Headset", 
                                            "MouseMat", "HeadsetStand", "CommanderPro",
                                            "LightingNodePro", "MemoryModule", "Cooler" };

const char* toString(CorsairError error) 
{
	switch (error) {
	case CE_Success :
		return "CE_Success";
	case CE_ServerNotFound:
		return "CE_ServerNotFound";
	case CE_NoControl:
		return "CE_NoControl";
	case CE_ProtocolHandshakeMissing:
		return "CE_ProtocolHandshakeMissing";
	case CE_IncompatibleProtocol:
		return "CE_IncompatibleProtocol";
	case CE_InvalidArguments:
		return "CE_InvalidArguments";
	default:
		return "unknown error";
	}
}

void init_nvml() {
		auto rc = nvmlInit();
	if (rc != NVML_SUCCESS) {
		cout << "Initializing NVML library failed: " << nvmlErrorString(rc) << endl;
	}
}

void init_icue() {
    CorsairPerformProtocolHandshake();
	if (const auto error = CorsairGetLastError()) {
		std::cout << "Handshake failed: " << toString(error) << "\nPress any key to quit." << std::endl;
		getchar();
        exit(-1);
	}
}

void report_error(string errorString) {
	CorsairError error = CorsairGetLastError();
	cout << "Corsair error while " << errorString << ": " << error << endl;
}

void print_device_info() {
   	auto colorsSet = std::unordered_set<int>();
    int size = CorsairGetDeviceCount();
    cout << "Found " << size << " devices" << endl;
	for (int deviceIdx = 0; deviceIdx < size; ++deviceIdx) {
        auto deviceInfo = CorsairGetDeviceInfo(deviceIdx);
        cout << "  Device ID " << deviceIdx << " is a " << DeviceTypeStrings[deviceInfo->type]
             << " with " << deviceInfo->ledsCount << " LEDs" << endl;
		if (const auto ledPositions = CorsairGetLedPositionsByDeviceIndex(deviceIdx)) {
			for (auto i = 0; i < ledPositions->numberOfLed; i++) {
				const auto ledId = ledPositions->pLedPosition[i].ledId;
				colorsSet.insert(ledId);
			}
		}
	}
    cout << endl;
}

LedMap get_led_arrays()
{
	LedMap ledMap;
	for (auto deviceIndex = 0; deviceIndex < CorsairGetDeviceCount(); deviceIndex++) {
		if (const auto ledPositions = CorsairGetLedPositionsByDeviceIndex(deviceIndex)) {
			vector<CorsairLedColor> leds;
			for (auto i = 0; i < ledPositions->numberOfLed; i++) {
				const auto ledId = ledPositions->pLedPosition[i].ledId;
				leds.push_back(CorsairLedColor{ ledId, 0, 0, 0 });
			}
			ledMap[deviceIndex] = leds;
		}
	}
	return ledMap;
}

// Load colors to show the binary representation of number
void load_device_colors_binary(string devName, int devIdx, unsigned int number, LedMap &ledMap) {
	DevInfoType dev = devInfo.at(devName)[devIdx];
	if (number >= pow(2, dev.ledCount)) {
		cout << "Can't represent " << number << " with only " << dev.ledCount << " LEDs!";
		return;
	}
	for (int i = 0; i < dev.ledCount; ++i) {
		if ((number >> (dev.ledCount - 1 - i)) & 0x00000001) {   // Isolate bit and test if it should be lit
			ledMap[dev.deviceIndex][dev.ledStartIndex + i].r = 0;
			ledMap[dev.deviceIndex][dev.ledStartIndex + i].g = 128;
			ledMap[dev.deviceIndex][dev.ledStartIndex + i].b = 0;
		}
	}
}

void load_device_colors_activity(string devName, int devIdx, int percentFull, LedMap &ledMap) {
	DevInfoType dev = devInfo.at(devName)[devIdx];
	int threshold = percentFull * dev.ledCount / 100;
	for (int i = dev.ledStartIndex; i < dev.ledStartIndex + dev.ledCount; ++i) {
		if (i - dev.ledStartIndex <= threshold) {
			                               // RAM is really bright, dim it down
			ledMap[dev.deviceIndex][i].r = (devName.compare("ram") == 0 ? 32 : 255);
			ledMap[dev.deviceIndex][i].g = 0;
			ledMap[dev.deviceIndex][i].b = 0;
		}
		else {
			ledMap[dev.deviceIndex][i].r = 0;
			ledMap[dev.deviceIndex][i].g = (devName.compare("ram") == 0 ? 32 : 255);
			ledMap[dev.deviceIndex][i].b = 0;
		}
	}
}

void set_colors(LedMap ledMap) {
	for (auto leds : ledMap) {
		auto deviceIdx = leds.first;
		auto ledsVec = leds.second;
		if (!CorsairSetLedsColorsBufferByDeviceIndex(deviceIdx, ledsVec.size(), ledsVec.data())) {
			report_error("setting DRAM LEDs");
		}
		if (!CorsairSetLedsColorsFlushBuffer()) {
			report_error("flushing DRAM LEDs");
		}
	}
}

int get_memory_usage() {
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
	DWORDLONG usedVirtualMem = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
//	cout << "usedVirtualMem = " << usedVirtualMem / (1024 * 1024 * 1024) << " GiB" << endl;
//	cout << "totalVirtualMem = " << totalVirtualMem / (1024 * 1024 * 1024) << " GiB" << endl;
	return static_cast<int> (usedVirtualMem * 100 / totalVirtualMem);
}

//
// CPU usage
//   Thanks - https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c
// 
static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

static float calculate_cpu_load(unsigned long long idleTicks, unsigned long long totalTicks) {
   static unsigned long long _previousTotalTicks = 0;
   static unsigned long long _previousIdleTicks = 0;
   unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
   unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

   float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);
   _previousTotalTicks = totalTicks;
   _previousIdleTicks  = idleTicks;
   return ret;
}

static unsigned long long file_time_to_int64(const FILETIME & ft) {
	return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);
}

int get_cpu_load() {
   FILETIME idleTime, kernelTime, userTime;
   int loadPct = 0;
   if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
	   loadPct = static_cast<int>(floor(100 * 
	   		calculate_cpu_load(file_time_to_int64(idleTime), 
	                           file_time_to_int64(kernelTime) + file_time_to_int64(userTime))));
   }
   return loadPct;
}

//
// GPU usage - Using NVIDIA CUDA API to get GPU usage
// References:
//     https://software.intel.com/en-us/forums/graphics-profiling-debugging-and-analysis/topic/277859  
//     https://docs.nvidia.com/deploy/pdf/NVML_API_Reference_Guide.pdf
//

static int get_gpu_load() {
	unsigned int deviceCount;
	nvmlReturn_t rc;
	if (NVML_SUCCESS != nvmlDeviceGetCount(&deviceCount)) {
		cout << "Get device count failed: " << nvmlErrorString(rc) << endl;
		return 1;
	}
	//cout << "Found " << deviceCount << " GPU Units" << endl;
	if (deviceCount != 1) {
		cout << "Program only supports 1 device, but " << deviceCount << " were found" << endl;
		return 2;
	}
	nvmlDevice_t device;
	if (NVML_SUCCESS != nvmlDeviceGetHandleByIndex(0, &device)) {
		cout << "Get devices from unit failed: " << nvmlErrorString(rc) << endl;
		return 3;
	}
	nvmlUtilization_t util;
	if (NVML_SUCCESS != nvmlDeviceGetUtilizationRates(device, &util)) {
		cout << "Get GPU utilization failed: " << nvmlErrorString(rc) << endl;
		return 4;
	}

	return static_cast<int>(util.gpu);
} 

int main() {
    vector<string> msg {"Hello", "C++", "World", "from", "VS Code"};

    cout << "Connecting to iCue..." << endl;
    init_icue();
	cout << "Initializing NVidia nvml..." << endl;
	init_nvml();
	cout << endl;

	// print_device_info();

	while(true) {
		time_t curTime = time(NULL);
		tm *time = localtime(&curTime);
		auto memPct = get_memory_usage();
		auto cpuPct = get_cpu_load();
		auto gpuPct = get_gpu_load();
		cout << "Time: " << setw(2) << setfill('0') << time->tm_hour 
		                 << ":" << setw(2) << setfill('0') << time->tm_min
						 << ":" << setw(2) << setfill('0') << time->tm_sec;
		cout << ", Memory: " << memPct << "%";
		cout << ", CPU:" << cpuPct << "%";
		cout << ", GPU:" << gpuPct << "%" << endl;
	 	LedMap ledMap = get_led_arrays();
		load_device_colors_activity("cpu", 0, cpuPct, ledMap);
		load_device_colors_activity("gpu", 0, gpuPct, ledMap);

	 	load_device_colors_activity("ram", 0,  min(memPct*4, 100), ledMap);
	 	load_device_colors_activity("ram", 1,  min((memPct-25)*4, 100), ledMap);
	 	load_device_colors_activity("ram", 2,  min((memPct-50)*4, 100), ledMap);
	 	load_device_colors_activity("ram", 3,  min((memPct-75)*4, 100), ledMap);

		// Show time on fans, hour (top), first digit of minute, second digit (bottom)
		load_device_colors_binary("fan", 0, time->tm_hour % 12, ledMap);
		load_device_colors_binary("fan", 1, time->tm_min / 10, ledMap);
		load_device_colors_binary("fan", 2, time->tm_min % 10, ledMap);

	 	set_colors(ledMap);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	// For debugging
	//cout << "Press enter to exit";
	//cin.get();
}
