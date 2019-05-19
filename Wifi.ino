#include <AltSoftSerial.h>

// TMP36 parameters
int tempSensorPin = A0;  // Temp sensor in pin0

// Lock parameters
int highLowPin = 12; //Pin to control lock 1
int highLowPin2 = 13; //Pin to control lock 2
int lock = 1;
int lock2 = 2;

bool DEBUG = true;   //show more logs
int responseTime = 200; //communication timeout
#define serialBaudrate 9600  // Baudrate for Arduino serial bus (PC<->Arduino)
#define wifiSerialBaudrate 9600 // Baudrate for wifiSerial (Arduino<->ESP8266 wifi module)

boolean timerOn = false; //timer set off
boolean lock1_open = false;
boolean lock2_open = false;
long partTime = 0;  //80% of given time
long leftTime = 0; // rest of the given time
long totalTime = 0; //total time to wait
unsigned long currentTime = 0; //Time when timer has set on

AltSoftSerial wifiSerial;      //  for ESP8266

void setup()
{
  pinMode(highLowPin,OUTPUT);
  pinMode(highLowPin2,OUTPUT);
  digitalWrite(highLowPin, HIGH);
  digitalWrite(highLowPin2, HIGH);
  // Open serial communications and wait for port to open esp8266:
  Serial.begin(serialBaudrate);
  while (!Serial) {
    ; // wait for serial port to connect.
  }
  sendToUno("Serial ready",responseTime,DEBUG);
  delay(1000);
  wifiSerial.begin(wifiSerialBaudrate);  
  sendToWifi("AT+CWMODE=2",responseTime,DEBUG); // configure as access point
  sendToWifi("AT+CWSAP=\"HeinaMatti\",\"\",5,0",responseTime,DEBUG); // configure as access point
  sendToWifi("AT+CIFSR",responseTime,DEBUG); // get ip address
  sendToWifi("AT+CIPMUX=1",responseTime,DEBUG); // configure for multiple connections
  sendToWifi("AT+CIPSERVER=1,80",responseTime,DEBUG); // turn on server on port 80
 
  if (wifiSerial.available()){
    Serial.println(wifiSerial.read());
  }
}

void loop()
{
  if(wifiSerial.available()>0){
    handleWifiMessage();
  }
  
  if (timerOn){                               
    if (lock1_open && lock2_open)
    {
      Serial.println("Both locks open!");
      // This should never happen!
    }
    if (elapsedTime() > totalTime){
      if (lock1_open)
      {
        openLock(lock2);
        lock2_open = true;
        leftTime = 0;
        timerOn = false;
      }
      else
      {
        openLock(lock);
        timerOn = false;
        leftTime = 0;
        lock1_open = true;
      }
    }
    else if (elapsedTime() >= partTime && readTemperature() <= -20){
      if (!lock1_open)
      {
        openLock(lock);
        lock1_open = true;
      }
    }
  } // timerOn
  delay(responseTime);
}
/*************
* Name: elapsedTime
* Description: Count differenc of current time and start time. Save time to left
* Params: 
* Returns: long
***************/
long elapsedTime(){
  long spentTime;
  spentTime = millis() - currentTime;
  leftTime = totalTime - spentTime;
  return spentTime;
}

/*************
* Name: handleWifiMessage
* Description: Handler reseived message
* Params: 
* Returns: 
***************/
void handleWifiMessage(){
  String message = readWifiSerialMessage();    
  if(find(message,"TEMP")){    //arduino sends temperature
    sendTemperature(readTemperature());
    }
  if(find(message,"TIMER_RESET")){ //Set timer off
    timerOn = false;
    leftTime = 0;
  }   
  if(find(message,"TIMER_GET")){ // asking if timer is already on
    if (timerOn == true){
      sendData("TimerON;");    //sending message that timer is on
    }
    else{
      sendData("TimerOFF;");    //sending message that timer is off
    }
  }
  if(find(message,"TIME_GET")){  //semd time what is left
    String str = "";
    str = "TIME_SET" + String(leftTime) + ";";
    sendData(str);
  }
  if(find(message,"TIMER_SET")){ // set new timer
    if (find(message,"time1")){
      calculateTime(4);    
    }
    else if(find(message,"time2")){ //receives timevalue from wifi
      calculateTime(6);
    }
    else if(find(message,"time3")){ //receives timevalue from wifi
      calculateTime(8);
    }
    else if(find(message,"time4")){ //receives timevalue from wifi
      calculateTime(10);
    }
    else if(find(message,"time5")){ //receives timevalue from wifi
      calculateTime(12);
    }
    else{
      sendData("Invalid timer!" + message + ";");
      return;
    }
    lock1_open = false;
    lock2_open = false;
  }
  // For testing only!
  if(find(message,"LOCK1")){
    //Serial.println("Open lock 1");
    openLock(1);
  }
  if(find(message,"LOCK2")){
    //Serial.println("Open lock 2");
    openLock(2);
  }
}

/*************
* Name: readTemperature
* Description: Function used to read temperature and convert result tu Celsius
* Params: 
* Returns: float
***************/
float readTemperature(){
  int adc = analogRead(tempSensorPin);
  int milliVolts = analogRead(tempSensorPin) * (5000/1024);
  double voltage = double(adc) * 5.0 / 1024;
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV 
  return temperatureC;                          //offset to degrees ((voltage - 500mV) times 100)
}

/************
* Name: sendTemperature
* Description: Function used to send temperature as string
* Params: 
* Returns: void
*************/
void sendTemperature(float temperatureC){
  String celsius;
  celsius = String(temperatureC);
  sendData("TEMP:" + celsius + ";");
}

/**************
* Name: sendData
* Description: Function used to send string to tcp client using cipsend
* Params: 
* Returns: void
*************/
void sendData(String str){
  String len="";
  len+=str.length();
  sendToWifi("AT+CIPSEND=0," + len, responseTime, DEBUG);
  delay(100);
  sendToWifi(str,responseTime,DEBUG);
  delay(100);
}

/*************
* Name: find
* Description: Function used to match two string
* Params: 
* Returns: true if match else false
***********/
boolean find(String string, String value){
  return string.indexOf(value)>=0;
}

/*******************
* Name: readSerialMessage
* Description: Function used to read data from Arduino Serial.
* Params: 
* Returns: The response from the Arduino (if there is a reponse)
******************/
String readSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(Serial.available()>0){
    value[index_count]=Serial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
  }
  String str(value);
  str.trim();
  return str;
}

/************
* Name: readWifiSerialMessage
* Description: Function used to read data from ESP8266 Serial.
* Params: 
* Returns: The response from the esp8266 (if there is a reponse)
***************/
String readWifiSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(wifiSerial.available()>0){
    value[index_count] = wifiSerial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
    delay(10);
  }
  String str(value);
  str.trim();
  return str;
}

/**************
* Name: sendToWifi
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
**************/
String sendToWifi(String command, const int timeout, boolean debug){
  String response = "";
  wifiSerial.println(command); // send the read character to the esp8266
  if (find(command, "AT+")){  // If sending AT command, read response
    long int time = millis();
    bool exit = false;
    while( (time+timeout) > millis())
    {
      if (exit){
        break;
      }
      while(wifiSerial.available())
      {
        // The esp has data so display its output to the serial window 
        char c = wifiSerial.read(); // read the next character.
        response += c;
        String str(response);
        if (find(str, "OK\r\n")){
          exit = true;
          break;
        }
      }  
    }
    if(debug)
    {
      Serial.println(response);
    }
  }
  return response;
}

/*************
* Name: sendToUno
* Description: Function used to send data to Arduino.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*****************/
String sendToUno(String command, const int timeout, boolean debug){
  String response = "";
  Serial.println(command); // send the read character to the Arduino
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(Serial.available())
    {
      // The esp has data so display its output to the serial window 
      char c = Serial.read(); // read the next character.
      response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}

/***********************
* Name: calculateTime
* Description: Function calculate 80% of given time
*
************************/
void calculateTime (long giventime)
{
  currentTime = millis();
  partTime = totalTime / 100 * 80;  //partTime is 80% of given time 
  timerOn = true;  // Set timer ON
}

/************************
* Name: openLock
* Description: Funtion send pulse to magneticlock and open it
* Returns: -
*************************/
int openLock(int lock){
  if (lock == 1){
    digitalWrite(highLowPin, LOW); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin, HIGH);  // sets power off to relay
    delay(2000);
  }else if (lock == 2){
    digitalWrite(highLowPin2, LOW); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin2, HIGH);  // sets power off to relay
    delay(2000);
    } 
}
 
 
