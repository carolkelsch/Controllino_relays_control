#include <Controllino.h>
#include "variables.h"

/*
  CONTROLLINO MEGA/MAXI - Control a motor with Relays 1, 2 3 and 4.
*/

// Pins configuration
#define RELAY_1           CONTROLLINO_R1
#define RELAY_2           CONTROLLINO_R2
#define RELAY_3           CONTROLLINO_R3
#define RELAY_4           CONTROLLINO_R4

// Pins for switchs
#define FUNC_MODE_PIN     CONTROLLINO_A0
#define TOP_SWITCH_PIN    CONTROLLINO_A1
#define BOTTOM_SWITCH_PIN CONTROLLINO_A2

/*
Comute the relay.
Possible values are defined by RELAY_#.
action -> open(1)/close(0)
*/
void comute_relay(int relay_pin, int action)
{
  if(action == OPEN){
    digitalWrite(relay_pin, LOW);
  }
  else if(action == CLOSE){
    digitalWrite(relay_pin, HIGH);
  }
}

/*
Switch relays to go up.
*/
void go_up()
{
  if(moving_state == GOING_DOWN){
    disable_outputs();
    delay(1000);
  }
  // Relays 1 and 3 are closed
  digitalWrite(RELAY_1, HIGH);
  components[0].value = CLOSE;
  digitalWrite(RELAY_3, HIGH);
  components[2].value = CLOSE;
  moving_state = GOING_UP;
}

/*
Switch relays to go down.
*/
void go_down()
{
  if(moving_state == GOING_UP){
    disable_outputs();
    delay(1000);
  }
  // Relays 2 and 4 are closed
  digitalWrite(RELAY_2, HIGH);
  components[1].value = CLOSE;
  digitalWrite(RELAY_4, HIGH);
  components[3].value = CLOSE;
  moving_state = GOING_DOWN;
}

/*
Disable all outputs for safety reason, in case the connection is lost.
*/
void disable_outputs()
{
  digitalWrite(RELAY_1, LOW);
  components[0].value = OPEN;
  digitalWrite(RELAY_2, LOW);
  components[1].value = OPEN;
  digitalWrite(RELAY_3, LOW);
  components[2].value = OPEN;
  digitalWrite(RELAY_4, LOW);
  components[3].value = OPEN;
  moving_state = STOPPED;
}

/*
CheckSum of values. Simple sum.
*/
uint8_t check_sum(char *value, int len)
{
  unsigned int index = 0;
  unsigned int check = 0;
  unsigned int csbyte = 0;

  // Sum the received bytes and compare to the last byte received
  if(len > 1){
    for(index = 0; index < (len-1); index++){
      check = check + *(value+index);
    }
    csbyte = *(value+(len-1));
    
    if(check == csbyte){
      return SUCCESS;
    }else if((check & 0x00FF) == (csbyte & 0x00FF)){
      return SUCCESS;
    }else{
      return FAILURE;
    }
  }
  else{
    return FAILURE;
  }
}

/*
Parse UDP package received.
*/
void parse_packet(int package_len)
{
  int ind = 0;
  int packet_count = 0;
  int comp = 0;
  int resp_status = 0;
  
  if(package_len == 5)
  {
    comp = (unsigned char)(packet[0]) + 0;
    if(comp == ACTUATE_SIMPLE) // Open or close command for only one relay
    {
      if(running_state == GENERIC)
      {
        // Check if it's not an optimized command
        comp = (unsigned char)(packet[2]) + 0;
        if(comp != 0){
          // Only work if it's in generic mode
          for(ind = 0; ind < 4; ind++)
          {
            comp = (unsigned char)(packet[2]) + 0;
            if(ind == (comp-1)){
              comp = (unsigned char)(packet[3]) + 0;
              comute_relay(components[ind].pin, comp);
              if(comp == CLOSE){
                components[ind].value = CLOSE;
              }
              else if(comp == OPEN){
                components[ind].value = OPEN;
              }
              reply[0] = (unsigned char)(packet[0]);
              reply[1] = ind + 1;
              reply[2] = components[ind].value;
              reply[3] = ACK;
              reply[4] = reply[0] + reply[1] +  reply[2] + reply[3];
              reply_len = 5;
              break;
            }
          }
        }
        else{
          reply[0] = NACK;
          reply_len = 1;
        }
      }
      else
      {
        comp = (unsigned char)(packet[2]) + 0;
        // Check if it's not a generic command
        if(comp == 0){
          // These only work in optimized mode
          comp = (unsigned char)(packet[3]) + 0;
          switch(comp){
            case STOP: // Stop motor
            disable_outputs();
            reply[0] = (unsigned char)(packet[0]);
            reply[1] = STOP;
            reply[2] = ACK;
            reply[3] = reply[0] + reply[1] + reply[2];
            reply_len = 4;
            break;
            
            case GO_DOWN: // Move stand down
            if(time_delay != 0){
              timer_counter = 0;
              go_down();
              TIMSK1 |= (1 << OCIE1A);  // Enable timer compare interrupt
            }
            else{ // If specified time is 0s, then go down until receive stop or up command
              go_down();
            }
            // Send ACK response
            reply[0] = (unsigned char)(packet[0]);
            reply[1] = GO_DOWN;
            reply[2] = ACK;
            reply[3] = reply[0] +  reply[1] +  reply[2];
            reply_len = 4;
            break;
            
            case GO_UP: // Move stand up
            if(time_delay != 0){
              timer_counter = 0;
              go_up();
              TIMSK1 |= (1 << OCIE1A);  // Enable timer compare interrupt
            }
            else{ // If specified time is 0s, then go up until receive stop or down command
              go_up();
            }
            // Send ACK response
            reply[0] = (unsigned char)(packet[0]);
            reply[1] = GO_UP;
            reply[2] = ACK;
            reply[3] = reply[0] +  reply[1] +  reply[2];
            reply_len = 4;
            break;
            
            default: // If the command was not recognized, send NACK response
            reply[0] = NACK;
            reply_len = 1;
            break;
          }
        }
        else{
          reply[0] = NACK;
          reply_len = 1;
        }
      }
    }
    else if(comp == ACTUATE_MULTIPLE) // Open or close command for multiple relays
    {
      if(running_state == GENERIC) // Only works if it's in generic mode
      {
        for(ind = 0; ind < 4; ind++)
        {
          comp = (unsigned char)(packet[2]) + 0;
          if(bitRead(comp, ind)){
            comp = (unsigned char)(packet[3]) + 0;
            comute_relay(components[ind].pin, bitRead(comp, ind));
            if(bitRead(comp, ind) == CLOSE){
              components[ind].value = CLOSE;
            }
            else if(bitRead(comp, ind) == OPEN){
              components[ind].value = OPEN;
            }
          }
        }
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = ((unsigned char)(packet[2]) & 0x0F);
        reply[2] = ((unsigned char)(packet[3]) & 0x0F);
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] +  reply[3];
        reply_len = 5;
      }
      else{ // If it's not in generic mode, rensponse is NACK
        reply[0] = NACK;
        reply_len = 1;
      }
    }
    else if(comp == MULTIPLE_REQUEST) // Request for multiple status on relays and switches
    {
      reply[0] = (unsigned char)(packet[0]);
      reply[1] = ((unsigned char)(packet[2]) & 0x0F);
      reply[2] = ((unsigned char)(packet[3]) & 0x07);

      // Get relays status
      reply[3] = 0;      
      for(ind = 0; ind < 4; ind++)
      {
        reply[3] = (reply[3] | (components[ind].value << ind));
      }
      
      // Get switches status
      reply[4] = 0;
      for(ind = 0; ind < 3; ind++)
      {
        reply[4] = (reply[4] | (components[ind+4].value << ind));
      }
      
      reply[5] = ACK;
      reply[6] = reply[0] + reply[1] + reply[2] + reply[3] + reply[4] + reply[5]; // Simple CS
      reply_len = 7;
    }
    else if(comp == PROGRAM_SETTINGS)
    {
      comp = (unsigned char)(packet[1]) + 0;
      if(comp == SET_RELAYS_DELAY) // Configure a new fixed time for optimized mode
      {
        unsigned int new_delay = ((unsigned char)(packet[2]) << 0x08) | (unsigned char)(packet[3]);
        
        disable_outputs();
        time_delay = new_delay; // Update fixed timing
        
        if(time_delay == 0){ // If time was set to 0s, it's because the continuous mode is selected, no timer is used in this mode
          disable_outputs();
          TIMSK1 = TIMSK1 & (0xFE << OCIE1A); // Disable timer compare interrupt
        }
        // Send response
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = SET_RELAYS_DELAY;
        reply[2] = ((new_delay >> 0x08) & (0xFF));
        reply[3] = (new_delay & 0xFF);
        reply[4] = ACK;
        reply[5] = char((reply[0] + reply[1] +  reply[2] + reply[3] + reply[4]) & 0xFF);
        reply_len = 6;
      }
      else if(comp == CHANGE_CODE) // Change working mode
      {
        comp = (unsigned char)(packet[3]) + 0;
        switch(comp){
          case GENERIC_CODE: // Run generic code
          running_state = GENERIC;
          disable_outputs();
          // Send response
          reply[0] = (unsigned char)(packet[0]);
          reply[1] = CHANGE_CODE;
          reply[2] = GENERIC_CODE;
          reply[3] = ACK;
          reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
          reply_len = 5;
          break;
          case OPTIMIZED_CODE: // Run optimized code
          running_state = OPTIMIZED;
          disable_outputs();
          // Send response
          reply[0] = (unsigned char)(packet[0]);
          reply[1] = CHANGE_CODE;
          reply[2] = OPTIMIZED_CODE;
          reply[3] = ACK;
          reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
          reply_len = 5;
          break;
          default: // Not recognized mode
          reply[0] = NACK;
          reply_len = 1;
          break;
        }
      }
      else{ // If the command was not recognized, send NACK
        reply[0] = NACK;
        reply_len = 1;
      }
    }
    else if(comp == SIMPLE_REQUEST) // Request for only a component status
    {
      comp = (unsigned char)(packet[3]) + 0;
      switch(comp){
        case CONNECTION: // Connection status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = CONNECTION;
        reply[2] = CONNECTION;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case RELAY1: // Get relay1 status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = RELAY1;
        reply[2] = components[0].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case RELAY2: // Get relay2 status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = RELAY2;
        reply[2] = components[1].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case RELAY3: // Get relay3 status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = RELAY3;
        reply[2] = components[2].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case RELAY4: // Get relay4 status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = RELAY4;
        reply[2] = components[3].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case FUNC_MODE: // Get functional mode status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = FUNC_MODE;
        reply[2] = components[4].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case TOP_SWITCH: // Get top end switch status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = TOP_SWITCH;
        reply[2] = components[5].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        case BOTTOM_SWITCH: // Get bottom end switch status
        reply[0] = (unsigned char)(packet[0]);
        reply[1] = BOTTOM_SWITCH;
        reply[2] = components[6].value;
        reply[3] = ACK;
        reply[4] = reply[0] +  reply[1] +  reply[2] + reply[3];
        reply_len = 5;
        break;
        default: // If component was not recognized, send NACK
        reply[0] = NACK;
        reply_len = 1;
        break;
      }
    }
  }
  else{ // If invalid size of packet, send NACK
    reply[0] = NACK;
    reply_len = 1;
  }
}

void setup() {
 // Configure the components of the system as relays and switches
  components[0].pin = RELAY_1;
  components[1].pin = RELAY_2;
  components[2].pin = RELAY_3;
  components[3].pin = RELAY_4;
  components[4].pin = FUNC_MODE_PIN;
  components[5].pin = TOP_SWITCH_PIN;
  components[6].pin = BOTTOM_SWITCH_PIN;
  
  // Configure pin modes as input or output
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);
  pinMode(FUNC_MODE_PIN, INPUT);
  pinMode(TOP_SWITCH_PIN, INPUT);
  pinMode(BOTTOM_SWITCH_PIN, INPUT);

  // Set starting values for each pin and update the last setted values
  disable_outputs();

  // Store initial status for switches
  components[4].value = (digitalRead(FUNC_MODE_PIN) ? CLOSE : OPEN);
  components[5].value = (digitalRead(TOP_SWITCH_PIN) ? CLOSE : OPEN);
  components[6].value = (digitalRead(BOTTOM_SWITCH_PIN) ? CLOSE : OPEN);

  // Start the serial port
  Serial.begin(115200);
  Serial.setTimeout(200);

  // Initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 250;            // compare match register 16MHz/64/1000Hz ->1ms
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS11);    // 64 prescaler
  TCCR1B |= (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
  TIMSK1 = TIMSK1 & (0xFE << OCIE1A);  // disable timer compare interrupt
}

/*
Timer callback for fixed time operating mode.
*/
ISR(TIMER1_COMPA_vect)
{
  timer_counter = timer_counter + 1;
  if(timer_counter >= time_delay){
    disable_outputs();
    TIMSK1 = TIMSK1 & (0xFE << OCIE1A); // Disable timer
  }
}

void loop() {
  // If Serial is available wait to get a full command
  int available_len = Serial.available();
  if(available_len > 0){
    int len = Serial.readBytes(packet, 5);

    if(check_sum(packet, len) == SUCCESS){
      parse_packet(len);
      // Send response packet
      Serial.write(reply, reply_len);
      Serial.flush();
    }
    else{
      // Send response packet
      reply[0] = NACK;
      reply_len = 1;
      Serial.write(reply, reply_len);
      Serial.flush();
    }
  }
  components[4].value = (digitalRead(FUNC_MODE_PIN) ? CLOSE : OPEN);
  components[5].value = (digitalRead(TOP_SWITCH_PIN) ? CLOSE : OPEN);
  components[6].value = (digitalRead(BOTTOM_SWITCH_PIN) ? CLOSE : OPEN);               
}
