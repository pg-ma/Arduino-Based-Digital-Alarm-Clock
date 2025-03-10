// Include necessary libraries
#include <Wire.h>
#include <at24c32.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <DigitLedDisplay.h>

// Constants and Definitions
#define CLOCK_INTERRUPT_PIN 2
#define MAX_ALARMS 12
#define MAX_CUSTOM_ALARMS 12
#define ALARM_SIZE 8 // Format: "HH:MMA/P"
const char phoneNumber[] = "+639162725339"; // Replace with your phone number

// RTC, SIM800L, and LED Display Setup
RTC_DS3231 rtc;
SoftwareSerial sim800l(10, 11); // RX, TX
DigitLedDisplay ld = DigitLedDisplay(7, 6, 5);
AT24C32 eprom(AT24C_ADDRESS_7);
int secondsLed = 8;
int RelayPin = 9;
String response="";

struct Alarm {
  int hour;      // Alarm hour
  int minute;    // Alarm minute
  char type;     // Alarm type ('S', 'N', 'L', 'R', 'D')
};

bool useCustom = false;
Alarm alarms[MAX_ALARMS]; // Array to store alarms
int alarmCount = 0;       // Number of alarms currently set

Alarm customAlarms[MAX_CUSTOM_ALARMS]; // Array for custom alarms
int customAlarmCount = 0;              // Number of custom alarms

int yr = 0, mth = 0, dayc = 0, hr = 0, min = 0, sec = 0;

// Function Prototypes
void initializeRTC();
void handleSMSCommand(String message);

// ---------------- Setup ----------------
void setup() {
  Serial.begin(9600); // Initialize serial communication

  ld.setBright(15);
  ld.setDigitLimit(5);
  startUpSeq();
  initializeRTC(); // Initialize RTC module
  initializeSIM800L();

  pinMode(secondsLed, OUTPUT);
  pinMode(RelayPin, OUTPUT);
  // Sample alarms
  // addAlarm(7, 30, 'S');  // Start of class at 8:00
  // addAlarm(8, 30, 'N'); // 8:30
  // addAlarm(9, 30, 'R'); // 9:30
  // addAlarm(9, 45, 'N');
  // addAlarm(10, 45, 'N');
  // addAlarm(11, 45, 'L');
  // addAlarm(12, 30, 'N');
  // addAlarm(13, 30, 'N');
  // addAlarm(14, 30, 'N');
  // addAlarm(15, 30, 'N'); 
  // addAlarm(16, 30, 'D'); // Dismissal at 16:30
  // displayAlarms(); // Show the list of alarms
  // Serial.println("System initialized.");
  // sendCommand("AT"); // Test communication
  // sendCommand("AT+CMGF=1"); // Set SMS to text mode
  // sendSMS("Hello! This is an automated message.");
  // int hour,hour1;

}

// ---------------- Main Loop ----------------
void loop() {
  if(sim800l.available()){
    processSIM800LMessages();
  }
  checkAndRingAlarms();
  displayCurrentTime(); // RTC function to display time
  digitalWrite(secondsLed, HIGH);
  delay(500);
  digitalWrite(secondsLed, LOW);
  delay(500);
}

void startUpSeq(){
  ld.printCharA(1);
  ld.printCharD(2);
  ld.printCharD(3);
  ld.printCharU(4);
  delay(3000);

  ld.printCharT(1);
  ld.printDigit(5, 1);
  ld.printDigit(5, 2);
  ld.printCharH(4);
  ld.printDigit(5, 4);
  delay(3000);

  ld.printCharC(1);
  ld.printCharP(2);
  ld.printCharE(3);
  ld.printDigit(3, 3);
  ld.printCharA(5);
  delay(3000);
}

// ---------------- RTC Initialization ----------------
// Function to initialize the RTC module
void initializeRTC() {
  Wire.begin();
    if (!rtc.begin()) {
        // Serial.println("Error: RTC not found!");
        while (1); // Halt the program if RTC is not detected
    }

    if (rtc.lostPower()) {
        // Serial.println("RTC lost power, resetting time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set RTC to compile time
    }

    // Disable unused features for efficiency
    rtc.disable32K(); 
    pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP); // Configure RTC interrupt pin
}

// Display the current RTC time
void displayCurrentTime() {
  DateTime now = rtc.now();

  // Extract time components
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  // Handle 12-hour format and AM/PM
  bool isPM = (hour >= 12);
  if (hour == 0) {
      hour = 12; // Midnight hour
  } else if (hour > 12) {
      hour -= 12; // Convert to 12-hour format
  }
  displayTime();

  // Print each digit of the time using printDigit
  ld.printDigit(hour / 10, 0);    // Tens place of hour
  ld.printDigit(hour % 10, 1);    // Ones place of hour
  ld.printDigit(minute / 10, 2);  // Tens place of minute
  ld.printDigit(minute % 10, 3);  // Ones place of minute

  // AM or PM
  if(isPM){
    ld.printCharP(5);
  } else {
    ld.printCharA(5);
  }
}
// ---------------- GSM Initialization ----------------
// Initialize the SIM800L module
void initializeSIM800L() {
  sim800l.begin(9600);
  delay(1000);

  sim800l.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  sim800l.println("AT+CNMI=1,2,0,0,0"); // Send new SMS directly to serial
  delay(1000);
  Serial.println("SIM800L initialized.");
}

// ---------------- GSM Functions --------------------
// Function to extract timestamp components and adjust RTC
void extractTimestampComponents(String smsData) {
  int firstQuote = smsData.indexOf("\",\"");
  int startIndex = smsData.indexOf("\",\"", firstQuote + 3) + 3; // Find second occurrence
  if (startIndex != -1 && smsData.length() > startIndex + 17) {
    String timestamp = smsData.substring(startIndex, startIndex + 17); // Extract full timestamp
    
    // Separate the components
    String yearStr = "20" + timestamp.substring(0, 2); // YY (assuming 20YY format)
    String monthStr = timestamp.substring(3, 5);
    String dayStr = timestamp.substring(6, 8);
    String hourStr = timestamp.substring(9, 11);
    String minuteStr = timestamp.substring(12, 14);
    String secondStr = timestamp.substring(15, 17);

    // Convert components to integers
    int year = yearStr.toInt();
    int month = monthStr.toInt();
    int day = dayStr.toInt();
    int hour = hourStr.toInt();
    int minute = minuteStr.toInt();
    int second = secondStr.toInt();
  
    yr = year;
    mth = month;
    dayc = day;
    hr = hour;
    min = minute;
    sec = second;
  } else {
    Serial.println("Timestamp not found.");
  }
}

// Process incoming SMS messages from SIM800L
void processSIM800LMessages() {
  String response= sim800l.readString();
  int index = response.indexOf("+CMT:");
  if (index != -1) {
    extractTimestampComponents(response);
    int messageStart = response.indexOf("\n", index) + 1;
    String message = response.substring(messageStart);
    message.trim();
    handleSMSCommand(message);  
  }
}

void addAlarm(int hour, int minute, char type) {
  if (alarmCount < MAX_ALARMS) {
    alarms[alarmCount].hour = hour;
    alarms[alarmCount].minute = minute;
    alarms[alarmCount].type = type;
    alarmCount++;
    Serial.println("Alarm added: " + String(hour) + ":" + String(minute) + " Type: " + type);
  } else {
    Serial.println("Error: Maximum number of alarms reached.");
  }
}

void displayAlarms() {
  Serial.println("Current Alarms:");
  for (int i = 0; i < alarmCount; i++) {
    String type;
    switch (alarms[i].type) {
      case 'S': type = "Start of Class"; break;
      case 'N': type = "Next Period"; break;
      case 'L': type = "Lunch Break"; break;
      case 'R': type = "Recess Break"; break;
      case 'D': type = "Dismissal"; break;
      default: type = "Unknown"; break;
    }
    Serial.println(String(i + 1) + ". " + String(alarms[i].hour) + ":" + 
                   String(alarms[i].minute) + " - " + type);
  }
}

void checkAndRingAlarms() {
  DateTime now = rtc.now();
  for (int i = 0; i < alarmCount; i++) {
    if (now.hour() == alarms[i].hour && now.minute() == alarms[i].minute && useCustom == false) {
      ld.printCharT(1);
      ld.printDigit(5, 1);
      ld.printDigit(5, 2);
      ld.printCharH(4);
      ld.printDigit(5, 4);
      ringAlarm(alarms[i].type);
      delay(60000); // Prevent multiple triggers within the same minute
    }

    if (now.hour() == customAlarms[i].hour && now.minute() == customAlarms[i].minute && useCustom == true) {
      ld.printCharT(1);
      ld.printDigit(5, 1);
      ld.printDigit(5, 2);
      ld.printCharH(4);
      ld.printDigit(5, 4);
      ringAlarm(customAlarms[i].type);
      delay(60000); // Prevent multiple triggers within the same minute
    }
  }
}

// Function to adjust the RTC time
void updateRTCDateTime(int year, int month, int day, int hour, int minute, int second) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.println("RTC adjusted to: " + String(year) + "-" + String(month) + "-" + String(day) + 
                   " " + String(hour) + ":" + String(minute) + ":" + String(second));
}

void ringAlarm(char alarmType) {
    switch (alarmType) {
        case 'S': // Start of Class
            startOfClass();
            break;
        case 'N': // Next Period
            NextPeriod();
            break;
        case 'L': // Lunch Break
            lunchBreak();
            break;
        case 'R': // Recess Break
            recessBreak();
            break;
        case 'D': // Dismissal
            dismissalBell();
            break;
        default:
            // sendSMS(responseNumber, "Error: Unknown alarm type.");
            break;
    }
}

void addCustomAlarm(int hour, int minute, char type) {
    int index=0;
    for(int i=0;i<MAX_CUSTOM_ALARMS;i++){
      if (customAlarms[i].hour==0 && customAlarms[i].minute==0){
      
        index=i;
        break;
      }
      else{
      }
    }
    
  customAlarms[index].hour = hour;
  customAlarms[index].minute = minute;
  customAlarms[index].type = type;  // Assign the provided type
  // Serial.println("Custom alarm added: " + String(hour) + ":" + String(minute) + " Type: " + type);
}

bool populateAlarmList(String input){
  int amountOfAlarms=customAlarmCount;
  int index=0;
  char target=':';
  bool successfull = true;
  while ((index = input.indexOf(target, index)) != -1) {
    if(amountOfAlarms >= MAX_CUSTOM_ALARMS){
      Serial.println("Max Alarms Reached");
      successfull = false;
      break;
    }
    String hourS=input.substring(index-2,index);
    int hour=hourS.toInt();
    if(hour < 0 || hour > 23) {
      Serial.println("Invalid Hour");
      successfull = false;
      break;
    }
    int minute=input.substring(index+1,index+3).toInt();
    if(minute < 0 || minute > 59) {
      Serial.println("Invalid Minute");
      successfull = false;
      break;
    }
    char type=input[index+3];
    if(type != 'S' && type != 'N' && type != 'R' && type != 'L' && type != 'D'){
      Serial.println("Invalid type of alarm");
      successfull = false;
      break;
    }
    Serial.println(type);
    index++; // Move to the next character to continue searching
    amountOfAlarms++;

    addCustomAlarm(hour, minute, type);
  }
  customAlarmCount = amountOfAlarms;
  Serial.println("customAlarmCount");
  Serial.println(customAlarmCount);
  saveToEEPROM();
  return successfull;
}

void saveToEEPROM() {
  for (int i = 0; i < customAlarmCount; i++) 
  {
    int address=i*10;
    int hour=customAlarms[i].hour;
    int minute=customAlarms[i].minute;
    char type=customAlarms[i].type;
    eprom.put(address,customAlarms[i].hour);
    eprom.put(address + 2,customAlarms[i].minute);
    eprom.put(address + 4,customAlarms[i].type);
  }
}

void loadFromEEPROM() {
  for (int i = 0; i < 500; i++) {
    int hour, minute;
    int address=i*10;
    char type;
    
    eprom.get(address, hour);
    eprom.get(address + 2, minute);
    eprom.get(address + 4, type);
    if(hour==0 && minute==0){
      Serial.println("customAlarmCount");
      Serial.println(customAlarmCount);
      break;
    }
    else{
      customAlarmCount++;
    }
    Serial.println("LOAD ADDRESS");
    Serial.println(address);
    Serial.println("LOAD HOUR");
    Serial.println(hour);
    Serial.println("LOAD MINUTE");
    Serial.println(minute);
    Serial.println("LOAD TYPE");
    Serial.println(type);
  }
  for (int i = 0; i < customAlarmCount; i++) {
    int hour, minute;
    int address=i*10;
    char type;
    Serial.println(address);
    eprom.get(address, hour);
    eprom.get(address + 2, minute);
    eprom.get(address + 4, type);
    Serial.println(hour);
    Serial.println(minute);
    Serial.println(type);
    addCustomAlarm(hour, minute, type);
  }
}

// ---------------- SMS Command Handling ----------------
// Function to handle SMS commands and provide feedback
void handleSMSCommand(String message) {
  message.trim(); // Remove any leading or trailing whitespace

  // Handle commands for preset alarms
  if (message.equalsIgnoreCase("WRONG TIME")) {
    // update RTC
    updateRTCDateTime(yr, mth, dayc, hr, min, sec);
  } 
  else if(message.startsWith("SET ALARM: ")){
    String alarms = message.substring(10);
    alarms.trim();

    if(populateAlarmList(alarms)){
      Serial.println("New Alarm/s Saved");
    }
    else{
      Serial.println("Errors Detected");
    }
  }
  //
  else if(message.equalsIgnoreCase("DELETE CUSTOM")){
    deleteAllAlarms();
    Serial.println("All custom alarms have been deleted");
  }
  else if(message.equalsIgnoreCase("SWITCH DEFAULT")){
    useCustom = false;
    Serial.println("Now using default alarms!");
  }
  else if(message.equalsIgnoreCase("SWITCH CUSTOM")){
    useCustom = true;
    Serial.println("Now using custom alarms!");
  }
  //
  else if(message.equalsIgnoreCase("VIEW ALARM")){
    viewAlarms();
  }
  else {
    Serial.println("Error: Unrecognized command.");
  }
}
//things to so
// delete custom alarms functions done
// wrong time check for every function/upload added done
// save alarm done
// view alarm


// ---------------- Bells ----------------
// Function for the start of class bell
void startOfClass() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(RelayPin, HIGH);
        delay(1500);
        digitalWrite(RelayPin, LOW);
        delay(1000);
    }
}

// Function for the dismissal bell
void dismissalBell() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(RelayPin, HIGH);
        delay(3000);
        digitalWrite(RelayPin, LOW);
        delay(1000);
    }
}

// Function for the lunch break bell
void lunchBreak() {
    for (int i = 0; i < 2; i++) {
        digitalWrite(RelayPin, HIGH);
        delay(2000);
        digitalWrite(RelayPin, LOW);
        delay(1500);
    }
}

// Function for the recess break bell
void recessBreak() {
    for (int i = 0; i < 2; i++) {
        digitalWrite(RelayPin, HIGH);
        delay(1500);
        digitalWrite(RelayPin, LOW);
        delay(2000);
        digitalWrite(RelayPin, HIGH);
        delay(1000);
        digitalWrite(RelayPin, LOW);
        delay(1000);
    }
}

// Function for the next period bell
void NextPeriod() {
    digitalWrite(RelayPin, HIGH);
    delay(3000);
    digitalWrite(RelayPin, LOW);

}

void displayTime() {
  char date[10] = "hh:mm:ss";
  rtc.now().toString(date);
  Serial.println(date);
}

void deleteAllAlarms(){
  for(int i=0;i<500;i++){
    eprom.put(i,0);
  }
  for(int i=0;i<12;i++){
    customAlarms[i].hour=0;
    customAlarms[i].minute=0;
    customAlarms[i].type=" ";
  }
}

void sendSMS(const char message[]) {
  Serial.println("Sending SMS...");

  // AT command to set recipient phone number
  sim800l.print("AT+CMGS=\"");
  sim800l.print(phoneNumber);
  sim800l.println("\"");
  delay(100);

  // Send the message body
  sim800l.print("message");
  delay(100);

  // Send Ctrl+Z to end the message
  sim800l.write(char(26));
  delay(5000); // Wait for the message to send

  Serial.println("SMS sent!");
}

void sendCommand(const char *command) {
  sim800l.println(command);
  delay(100);
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
}
void viewAlarms(){
  for(int i=0;i<customAlarmCount;i++){
    Serial.print("Hour: ");
    Serial.println(customAlarms[i].hour);
    Serial.print("Minute: ");
    Serial.println(customAlarms[i].minute);
    Serial.print("Type: ");
    Serial.println(customAlarms[i].type);
  }
}
