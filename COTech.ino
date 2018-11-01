/*
******************************************************************************************************************************************************************
*
* Clas Ohlson CO Tech power plugs
* Tested with remote ? and power plug ?
* 
* Code by Antti Kirjavainen (antti.kirjavainen [_at_] gmail.com)
* 
* https://www.clasohlson.com
* 
*
* HOW TO USE
* 
* Capture your remote control buttons (note that each button sends 4 different codes and at least 2 of them are needed) with
* RemoteCapture.ino and copy paste the 24 bit commands to COTech.ino for sendCOTechCommand(). More info about this provided
* in RemoteCapture.ino.
* 
* 
* HOW TO USE WITH EXAMPLE COMMANDS
* 
* 1. Set the plug into pairing mode by holding down its power button until the LED starts blinking.
* 2. Send a command, eg. "sendCOTechCommand(SWITCH_A_ON);", which stops the pairing mode.
* 3. Now you can control the plug, e.g. sendCOTechCommand(SWITCH_A_ON); (or SWITCH_A_OFF).
* 
* 
* PROTOCOL DESCRIPTION
*
* Commands bits are essentially quadrobits:
* A single command is: 1 AGC bit + 24 command quadrobits + radio silence
* 
* All sample counts below listed with a sample rate of 44100 Hz (sample count / 44100 = microseconds).
* 
* First 4 command repetitions have an AGC bit of:
* HIGH of approx. 328 samples = 7437 us
* 
* Last 4 repetitions have an AGC bit of (and no radio silence in between):
* HIGH of 110 approx. samples = 2494 us
*
* Pulse length:
* One data quadrobit: 68 approx. samples = 1542 us
*
* Data bits:
* Data 0 = LOW-HIGH-HIGH-HIGH (wire 0111)
* Data 1 = LOW-LOW-HIGH-HIGH (wire 0011)
* 
* One command is repeated 5 times at first, then the next command in order is repeated 4 + 5 times.
*
* End first 3 repetitions and final 8th repetition with LOW radio silence of 130 samples = 2948 us.
* 4th, 5th, 6th and 7th repetitions have no radio silence after data bits.
* 
******************************************************************************************************************************************************************
*/



const String SWITCH_A_ON[4]  = {"111000001111111100011100", "111011000001111001011100", "111001010010100010101100", "111000010110001001101100"};
const String SWITCH_A_OFF[4] = {"111010001100100110111100", "111011110111110001111100", "111000100000011100101100", "111001000101101010001100"};

/*
// These arrays are a memory hog. Comment extra commands out when not needed:

const String SWITCH_B_ON[4]  = {"111010111000010000110101", "111001110100110111110101", "111010101110101111100101", "111010011101000100000101"};
const String SWITCH_B_OFF[4] = {"111000111011000010010101", "111011100011001111010101", "111011011001010101000101", "111001101010011011000101"};

const String SWITCH_C_ON[4]  = {"111001000101101010001110", "111010001100100110111110", "111011110111110001111110", "111000100000011100101110"};
const String SWITCH_C_OFF[4] = {"111011000001111001011110", "111001000101101010001110", "111000010110001001101110", "111000001111111100011110"};

const String SWITCH_D_ON[4]  = {"111011011001010101000111", "111001101010011011000111", "111000111011000010010111", "111011100011001111010111"};
const String SWITCH_D_OFF[4] = {"111001110100110111110111", "111001101010011011000111", "111000111011000010010111", "111010111000010000110111"};
*/



#define TRANSMIT_PIN          13      // We'll use digital 13 for transmitting

// If you wish to use PORTB commands instead of digitalWrite, these are for Arduino Uno digital 13:
#define D13high | 0x20; 
#define D13low  & 0xDF; 

// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define COTECH_AGC1_PULSE                   7350  // 328 samples, between first 4 repetitions
#define COTECH_AGC2_PULSE                   2480  // 110 samples, between last 4 repetitions
#define COTECH_RADIO_SILENCE                2948  // 130 samples, only between first 3 repetitions and final 9th repetition

#define COTECH_PULSE_TIMES_TWO_LOW          910   // 38 samples, first bit of data 0
#define COTECH_PULSE_TIMES_TWO_HIGH         645   // 30 samples, second bit of data 0

#define COTECH_PULSE_SHORT                  400   // 17 samples, first bit of data 1
#define COTECH_PULSE_TIMES_THREE            1125  // 51 samples, second bit of data 1

#define COTECH_COMMAND_BIT_ARRAY_SIZE       24    // Command bit count



// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600); // Used for error messages even with DEBUG set to false
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  sendCOTechCommand(SWITCH_A_ON);
  delay(3000);
  sendCOTechCommand(SWITCH_A_OFF);
  delay(3000);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendCOTechCommand(String *command_list) {

  String command1 = "";
  String command2 = "";
  int next_command_pos = 0;
  
  // Declare the array (int) of command bits
  int command_array1[COTECH_COMMAND_BIT_ARRAY_SIZE];
  int command_array2[COTECH_COMMAND_BIT_ARRAY_SIZE];

  // Prepare for transmitting and check for validity
  pinMode(TRANSMIT_PIN, OUTPUT); // Prepare the digital pin for output
  transmitWaveformLow(1);

  // CO Tech switches need at least 2 of the 4 different codes for each command:
  for (int c = 0; c < 4; c++) {

    // Prepare two commands for sending:
    next_command_pos = c + 1;
    if (next_command_pos > 3) next_command_pos = 0;

    command1 = command_list[c];
    command2 = command_list[next_command_pos];
  
    // Processing a string during transmit is just too slow,
    // let's convert it to an array of int first:
    convertStringToArrayOfInt(command1, command_array1, COTECH_COMMAND_BIT_ARRAY_SIZE);
    convertStringToArrayOfInt(command2, command_array2, COTECH_COMMAND_BIT_ARRAY_SIZE);
    
    // First 3 repetitions, long AGC and full radio silence:
    for (int i = 0; i < 3; i++) {
      doCotechQuadrobitSend(command_array1, COTECH_COMMAND_BIT_ARRAY_SIZE, COTECH_AGC1_PULSE, COTECH_RADIO_SILENCE);
    }

    // 4th repetition, same AGC but only very short radio silence at the end (same as the short pulse):
    doCotechQuadrobitSend(command_array1, COTECH_COMMAND_BIT_ARRAY_SIZE, COTECH_AGC1_PULSE, COTECH_PULSE_SHORT);

    // Last 4 repetitions, next command, short AGC and short pulse radio silence at the end:
    for (int i = 0; i < 4; i++) {
        doCotechQuadrobitSend(command_array2, COTECH_COMMAND_BIT_ARRAY_SIZE, COTECH_AGC2_PULSE, COTECH_PULSE_SHORT);
    }

    // Radio silence in between:
    transmitWaveformLow(COTECH_RADIO_SILENCE - COTECH_PULSE_SHORT);

    // Disable output to transmitter to prevent interference with
    // other devices. Otherwise the transmitter will keep on transmitting,
    // which will disrupt most appliances operating on the 433.92MHz band:
    digitalWrite(TRANSMIT_PIN, LOW);
  }
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void doCotechQuadrobitSend(int *command_array, int command_array_size, int pulse_agc, int pulse_radio_silence) {

  if (command_array == NULL) {
    errorLog("doCotechQuadrobitSend(): Array pointer was NULL, cannot continue.");
    return;
  }

  // Starting (AGC) bit:
  transmitWaveformHigh(pulse_agc);
 
  // Transmit command:
  for (int i = 0; i < command_array_size; i++) {

      // If current bit is 0, transmit LOW-HIGH-HIGH-HIGH (0111):
      if (command_array[i] == 0) {
        transmitWaveformLow(COTECH_PULSE_SHORT);
        transmitWaveformHigh(COTECH_PULSE_TIMES_THREE);
      }
         
      // If current bit is 1, transmit LOW-LOW-HIGH-HIGH (0011):
      if (command_array[i] == 1) {
        transmitWaveformLow(COTECH_PULSE_TIMES_TWO_LOW);
        transmitWaveformHigh(COTECH_PULSE_TIMES_TWO_HIGH);
      }
   }

  // Radio silence at the end of command.
  // It's better to rather go a bit over than under required length.
  transmitWaveformLow(pulse_radio_silence);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitWaveformHigh(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, LOW); // Digital pin low transmits a high waveform
  //PORTB = PORTB D13low; // If you wish to use faster PORTB commands instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitWaveformLow(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, HIGH); // Digital pin high transmits a low waveform
  //PORTB = PORTB D13high; // If you wish to use faster PORTB commands instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
int convertStringToInt(String s) {
  char carray[2];
  int i = 0;
  
  s.toCharArray(carray, sizeof(carray));
  i = atoi(carray);

  return i;
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void convertStringToArrayOfInt(String command, int *int_array, int command_array_size) {
  String c = "";

  if (int_array == NULL) {
    errorLog("convertStringToArrayOfInt(): Array pointer was NULL, cannot continue.");
    return;
  }
 
  for (int i = 0; i < command_array_size; i++) {
      c = command.substring(i, i + 1);

      if (c == "0" || c == "1" || c == "2" || c == "3") { // 2 and 3 are allowed for other projects of mine (not used by this code)
        int_array[i] = convertStringToInt(c);
      } else {
        errorLog("convertStringToArrayOfInt(): Invalid character " + c + " in command.");
        return;
      }
  }
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void errorLog(String message) {
  Serial.println(message);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
