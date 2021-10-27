#include "defines.h"

/************************* Global Variables *************************/

// Component status variables
struct components_s{
  int pin;
  int value;
};
components_s components[7];

// State of running code
enum running_code_state{
  GENERIC = 0,
  OPTIMIZED,
};

running_code_state running_state = OPTIMIZED;

// State of test stand
enum test_stand_moving_state{
  STOPPED = 0,
  GOING_UP,
  GOING_DOWN,
};

test_stand_moving_state moving_state = STOPPED;


// Serial communication variables
char packet[255];
char reply[20] = "Packet received!";
int reply_len = 0;

// Delay for fixed time mode
int time_delay_1 = 5;
int time_delay_2 = 0;

int timer_counter_1 = 0;
int timer_counter_2 = 0;
