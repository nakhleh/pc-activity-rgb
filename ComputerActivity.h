#ifndef __ComputerActivity_h__
#define __ComputerActivity_h__

#include "windows.h"

using namespace std;

class ComputerActivity {
	// Used for calculating running CPU totals
	unsigned long long _previousTotalTicks = 0;
    unsigned long long _previousIdleTicks = 0;

	// Support CPU calculations
	float calculate_cpu_load(unsigned long long, unsigned long long);
	unsigned long long file_time_to_int64(const FILETIME &);

  public:
	ComputerActivity();
	int get_memory_usage();
	int get_cpu_load();
	int get_gpu_load();
};

#endif
