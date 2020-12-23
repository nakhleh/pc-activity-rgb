#include <iostream>
#include <iomanip>
#include <math.h>

extern "C" {
	#include "nvml.h"
}

#include "ComputerActivity.h"

using namespace std;

ComputerActivity::ComputerActivity() {
	cout << "Initializing NVML..." << endl;
	auto rc = nvmlInit();
	if (rc != NVML_SUCCESS) {
		cout << "Initializing NVML library failed: " << nvmlErrorString(rc) << endl;
	}
}

int ComputerActivity::get_memory_usage() {
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
float ComputerActivity::calculate_cpu_load(unsigned long long idleTicks, unsigned long long totalTicks) {
   unsigned long long totalTicksSinceLastTime = totalTicks - this->_previousTotalTicks;
   unsigned long long idleTicksSinceLastTime  = idleTicks - this->_previousIdleTicks;

   float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);
   this->_previousTotalTicks = totalTicks;
   this->_previousIdleTicks  = idleTicks;
   return ret;
}

unsigned long long ComputerActivity::file_time_to_int64(const FILETIME & ft) {
	return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);
}

int ComputerActivity::get_cpu_load() {
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
int ComputerActivity::get_gpu_load() {
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

