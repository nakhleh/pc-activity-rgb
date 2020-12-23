#ifndef __RgbLighting_h__
#define __RgbLighting_h__

#include <string>
#include <unordered_map>
#include <vector>

// iCUE API for controlling lighting
#include <CUESDK.h>

using namespace std;

using LedMap = unordered_map<int, vector<CorsairLedColor>>;

class RgbLighting {
	void report_error(string errorString);
	const char* toString(CorsairError error);
  public:
  	RgbLighting();
	void print_device_info();
	LedMap get_led_arrays();
	std::unordered_map<string, int> get_device_mapping();
	void load_device_colors_binary(string devName, int devIndex, int controllerIndex, unsigned int number, LedMap& ledMap);
	void load_device_colors_activity(string devName, int devIndex, int controllerIndex, int percentFull, LedMap &ledMap);
	void set_colors(LedMap ledMap);
};

#endif
