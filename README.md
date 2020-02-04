# pc-activity-rgb

Controls PC LEDs to visually represent system activity.

Requires the following external dependencies to work:

* [iCue SDK](https://forum.corsair.com/v3/forumdisplay.php?f=300) - Corsair's API for controlling LEDS
* [CUDA SDK](https://developer.nvidia.com/cuda-downloads) - used to get GPU utilization for NVidia GPUs

This was built on Windows using g++, installed through [scoop](https://scoop.sh/), and is set up
for editing and building via [vscodium](https://go.microsoft.com/fwlink/?LinkId=733558). Update
the tasks.json and c_cpp_properties.json files to point to the location of the external SDKs on
your system.


