// Please format this file with clang before check-in to GitHub
/*
  Serial_Read_Command - example to read and process commands sent via USB port

  Usage:
  1. Open the Arduino IDE
  2. Compile and load this sketch onto almost any Arduino
  3. Use the Arduino IDE to open the monitor window (Tools > Serial Monitor)
  4. Type commands into the serial monitor's top bar, e.g.:
     help     (Arduino replies with "Available commands are: help, version, send, data, dump")
     version  (Arduino replies with its program name, version and compile date)

  Created:  2021-11-29
  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA
*/

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "Serial_Read_Command"
#define PROGRAM_VERSION  "v1.07"
#define PROGRAM_LINE1    "Barry K7BWH, barry@k7bwh.com"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

// ----- table of commands
typedef void (*Function)();   // ptr to function with no arguments, void return

struct Command {
  char text[8];
  Function function;
};
Command cmdList[] = {
    {"help", help},
    {"version", version},
    {"send", send_message},
    {"data", get_data},
    {"dump", dump},
};
const int numCmds = sizeof(cmdList) / sizeof(cmdList[0]);

// ----- functions to implement commands
void help() {
  Serial.print("Available commands are: ");
  for (int ii = 0; ii < numCmds; ii++) {
    if (ii > 0) {
      Serial.print(", ");
    }
    Serial.print(cmdList[ii].text);
  }
  Serial.println();
}
void version() {
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);
  Serial.println("Compiled " PROGRAM_COMPILED);
  Serial.println(PROGRAM_LINE1);
  Serial.println(__FILE__);
}
void send_message() {
  Serial.println("send_message()");
}
void get_data() {
  Serial.println("get_data()");
}
void dump() {
  Serial.println("dump()");
}

//=========== setup ============================================
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);   // initialize the onboard LED so we can flash it

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  help();
  Serial.println("Ready");
}

//=========== main work loop ===================================
void loop() {
  if (Serial.available()) {
    digitalWrite(LED_BUILTIN, HIGH);   // indicate time waiting for string
    String command = Serial.readStringUntil('\n');
    delay(100);                       // watch the red LED...
    digitalWrite(LED_BUILTIN, LOW);   // ...if it stays ON a long time then the mainline loop is delayed

    char cmd[24];                            // convert "String" type to character array
    command.toCharArray(cmd, sizeof(cmd));   // because it's generally safer programming
    Serial.print(cmd);
    Serial.print(": ");

    bool found = false;
    for (int ii = 0; ii < numCmds; ii++) {        // loop through table of commands
      if (strcmp(cmd, cmdList[ii].text) == 0) {   // look for it
        cmdList[ii].function();                   // found it! call the subroutine
        found = true;
        break;
      }
    }
    if (!found) {
      Serial.println("Unsupported");
    }
  }
}
