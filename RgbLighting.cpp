#include <iostream>
#include <iomanip>
#include <math.h>
#include <unordered_set>
#include <vector>

#include "RgbLighting.h"

using namespace std;

struct DevInfoType {
	string deviceName;
	int ledCount;
	int ledStartIndex;
};

// Maps logical devices to physical iCUE indicies
static const unordered_map<string, vector<DevInfoType>> devInfo = {
	{ "gpu", {
				{"CommanderPro", 16,  0}
	}},
	{ "reservoir", {
				{"CommanderPro", 10, 16}
	}},
	{ "cpu", {
				{"CommanderPro", 16, 26}
	}},
	{ "fan", { 
				{"CommanderPro", 4, 42},
				{"CommanderPro", 4, 50},
				{"CommanderPro", 4, 46}
	}},
	{ "ram", {
				{"MemoryModule", 10, 0}
	}}
};

static const char* DeviceTypeStrings[] = { "Unknown", "Mouse", "Keyboard", "Headset", 
                                           "MouseMat", "HeadsetStand", "CommanderPro",
                                           "LightingNodePro", "MemoryModule", "Cooler" };

//
// Public methods
//

RgbLighting::RgbLighting() {
	// Init the iCue connection
    CorsairPerformProtocolHandshake();
	if (const auto error = CorsairGetLastError()) {
		std::cout << "Handshake failed: " << toString(error) << "\nPress any key to quit." << std::endl;
		getchar();
        exit(-1);
	}
}

void RgbLighting::print_device_info() {
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

// Apparently the devices don't always have the same mapping (upon driver updates?), so figure out which is the commander
// RAM sticks always seem to be in the correct order, though, so assume that
std::unordered_map<string, int> RgbLighting::get_device_mapping() {
   	auto deviceMap = std::unordered_map<string, int>();
	int ramNumber = 1;
    int size = CorsairGetDeviceCount();
    cout << "Found " << size << " devices" << endl;
	for (int deviceIdx = 0; deviceIdx < size; ++deviceIdx) {
        auto deviceInfo = CorsairGetDeviceInfo(deviceIdx);
		std::string deviceName(DeviceTypeStrings[deviceInfo->type]);
        cout << "  Device ID " << deviceIdx << " is a " << deviceName
             << " with " << deviceInfo->ledsCount << " LEDs" << endl;
		if (deviceName.compare("CommanderPro") == 0) {
			deviceMap.insert({DeviceTypeStrings[deviceInfo->type], deviceIdx});
		}
		else if (deviceName.compare("MemoryModule") == 0) {
			if (deviceMap.count("MemoryModule") == 0) {
				deviceMap.insert({"MemoryModule", deviceIdx});
				deviceMap.insert({"MemoryModuleCount", 1});
			}
			else {
				deviceMap.insert({"MemoryModuleCount", deviceMap["MemoryModuleCount"] + 1});
			}
		}
		else {
			cout << "Ignoring unrecognized device string: " << deviceName << endl;
		}
	}
    cout << endl;
	return deviceMap;
}

LedMap RgbLighting::get_led_arrays()
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
void RgbLighting::load_device_colors_binary(string devName, int devIndex, int controllerIndex, unsigned int number, LedMap &ledMap) {
	DevInfoType dev = devInfo.at(devName)[devIndex];
	if (number >= pow(2, dev.ledCount)) {
		cout << "Can't represent " << number << " with only " << dev.ledCount << " LEDs!";
		return;
	}
	for (int i = 0; i < dev.ledCount; ++i) {
		if ((number >> (dev.ledCount - 1 - i)) & 0x00000001) {   // Isolate bit and test if it should be lit
			ledMap[controllerIndex][dev.ledStartIndex + i].r = 0;
			ledMap[controllerIndex][dev.ledStartIndex + i].g = 128;
			ledMap[controllerIndex][dev.ledStartIndex + i].b = 0;
		}
	}
}

void RgbLighting::load_device_colors_activity(string devName, int devIndex, int controllerIndex, int percentFull, LedMap &ledMap) {
	DevInfoType dev = devInfo.at(devName)[devIndex];
	int threshold = percentFull * dev.ledCount / 100;
	for (int i = dev.ledStartIndex; i < dev.ledStartIndex + dev.ledCount; ++i) {
		if (i - dev.ledStartIndex <= threshold) {
			                               // RAM is really bright, dim it down
			ledMap[controllerIndex][i].r = (devName.compare("ram") == 0 ? 32 : 255);
			ledMap[controllerIndex][i].g = 0;
			ledMap[controllerIndex][i].b = 0;
		}
		else {
			ledMap[controllerIndex][i].r = 0;
			ledMap[controllerIndex][i].g = (devName.compare("ram") == 0 ? 32 : 255);
			ledMap[controllerIndex][i].b = 0;
		}
	}
}

void RgbLighting::set_colors(LedMap ledMap) {
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

//
// Private methods
//

void RgbLighting::report_error(string errorString) {
	CorsairError error = CorsairGetLastError();
	cout << "Corsair error while " << errorString << ": " << error << endl;
}

const char* RgbLighting::toString(CorsairError error) {
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






