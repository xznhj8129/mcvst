#include "globals.h"
#include "structs.h"

// Globals
bool global_debug_print = true;
std::atomic<bool> global_running(true); 
std::atomic<int> latestKeyCode{0}; // Use 0 to indicate no key press

