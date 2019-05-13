#include <AltSoftSerial.h>

// TMP36 parameters
int tempSensorPin = A0;  // Temp sensor in pin0

// Lock parameters
int highLowPin = 12; //Pin to set High and 
int highLowPin2 = 13;
int lock = 1;
int lock2 = 2;

bool DEBUG = true;   //show more logs
int responseTime = 100; //communication timeout
#define serialBaudrate 115200  // Baudrate for Arduino serial bus (PC<->Arduino)
#define wifiSerialBaudrate 9600 // Baudrate for wifiSerial (Arduino<->ESP8266 wifi module)

boolean timerOn = false; //timer set off
boolean lock1_open = false;
boolean lock2_open = false;
int partTime = 0;  //80% of given time
int leftTime = 0; // rest of the given time
int totalTime = 0; //total time to wait
unsigned long currentTime = 0; 

AltSoftSerial wifiSerial;      //  for ESP8266

void setup()
{
  pinMode(highLowPin,OUTPUT);
  // Open serial communications and wait for port to open esp8266:
  Serial.begin(serialBaudrate);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  sendToUno("Serial ready",responseTime,DEBUG);
  delay(1000);
  wifiSerial.begin(wifiSerialBaudrate);
  
  
  sendToUno("WifiSerial ready",responseTime,DEBUG);

  sendToWifi("AT+CWMODE=2",responseTime,DEBUG); // configure as access point
  sendToWifi("AT+CIFSR",responseTime,DEBUG); // get ip address
  sendToWifi("AT+CIPMUX=1",responseTime,DEBUG); // configure for multiple connections
  sendToWifi("AT+CIPSERVER=1,80",responseTime,DEBUG); // turn on server on port 80

 
  if (wifiSerial.available()){
    Serial.println(wifiSerial.read());
  }
 
  sendToUno("Wifi connection is running!",responseTime,DEBUG);
  

}


void loop()
{
  if(Serial.available()>0){
    handleSerialMessage();
  }
  if(wifiSerial.available()>0){
    handleWifiMessage();
  }

  if (timerOn){                               //int partTime 80% of given time int totalTime all of the given time
    if (lock1_open && lock2_open)
    {
      // This should never happen!
    }
    if (elapsedTime() > totalTime){
      if (lock1_open)
      {
        openLock(lock2);
        lock2_open = true;
        timerOn=(false);
      }
      else
      {
        openLock(lock);
        timerOn=(false);
        lock1_open = true;
      }
    }
    else if (elapsedTime() == partTime && readTemperature() < -20){
      if (!lock1_open)
      {
        openLock(lock);
        lock1_open = true;
      }
    }
  }
  delay(responseTime);
}

int elapsedTime(){

  unsigned long now = millis() ;
  int spentTime;
  spentTime = (int) now - (int) currentTime;
  leftTime = spentTime;
  return (spentTime); 
  
}

void handleSerialMessage(){
  String message = readSerialMessage();
  Serial.print("USB serialilta: ");
  Serial.println(message);
  if(find(message,"debugEsp8266:")){
    Serial.println("Lähetetään WiFi serialille");
    String result = sendToWifi(message.substring(13,message.length()),responseTime,DEBUG);
    Serial.print("Response: ");
    Serial.println(result);
    if(find(result,"OK")){
      sendData("\nOK");
    }
    else{
      sendData("\nEr");
    }
  }
}

void handleWifiMessage(){
  String message = readWifiSerialMessage();
  Serial.print("Received from WiFi: ");
  Serial.println(message);
    
  if(find(message,"esp8266:")){
    String result = sendToWifi(message.substring(8,message.length()),responseTime,DEBUG);
    Serial.print("Sending command to ESP8266");
    if(find(result,"OK"))
      sendData("\n"+result);
    else
      sendData("\nError reading message");               //At command ERROR CODE for Failed Executing statement
    }
  else if(find(message,"HELLO")){  //receives HELLO from wifi
    Serial.println("HI!");    //arduino says HI
    sendData("HI!");
  }
  else if(find(message,"TEMP")){
    Serial.println("TEMP:22");    //arduino sends temperature
    readTemperature;
    }
  else if(find(message,"Nollaus")){ //Set timer off
    timerOn = false;
    calculateTime(0);
  }   
  else if(find(message,"TIMER_GET")){ // asking if timer is already on
    if (timerOn == true){
      sendData("TimerON");    //sending message that timer is on
    }
    else{
      sendData("TimerOFF");    //sending message that timer is off
    }
  }
  else if(find(message,"TIME_GET")){  //semd time what is left
    sendData("TIME_SET"+leftTime);
    
  }
  else if(find(message,"TimerSet")){ // set new timer
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
  }
  else{
    Serial.print("\nUnknown command!");
    Serial.println(message);
    sendData("\nUnknown command!");                 //Command ERROR CODE for UNABLE TO READ 
  }
}

float readTemperature(){
  int adc = analogRead(tempSensorPin);
  // Serial.println(adc);
  int milliVolts = analogRead(tempSensorPin) * (5000/1024);
  double voltage = double(adc) * 5.0 / 1024;
  // Serial.println(voltage);
  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                                //to degrees ((voltage - 500mV) times 100)
  String celsius;
  celsius = String(temperatureC);
  
  Serial.print(temperatureC);
  Serial.println(" degrees C");
  sendData("temp:"+celsius);
  return (temperatureC);
}

/*
* Name: sendData
* Description: Function used to send string to tcp client using cipsend
* Params: 
* Returns: void
*/
void sendData(String str){
  String len="";
  len+=str.length();
  sendToWifi("AT+CIPSEND=0,"+len,responseTime,DEBUG);
  delay(100);
  sendToWifi(str,responseTime,DEBUG);
  delay(100);
  //sendToWifi("AT+CIPCLOSE=5",responseTime,DEBUG);
}


/*
* Name: find
* Description: Function used to match two string
* Params: 
* Returns: true if match else false
*/
boolean find(String string, String value){
  return string.indexOf(value)>=0;
}


/*
* Name: readSerialMessage
* Description: Function used to read data from Arduino Serial.
* Params: 
* Returns: The response from the Arduino (if there is a reponse)
*/
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



/*
* Name: readWifiSerialMessage
* Description: Function used to read data from ESP8266 Serial.
* Params: 
* Returns: The response from the esp8266 (if there is a reponse)
*/
String readWifiSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(wifiSerial.available()>0){
    value[index_count]=wifiSerial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
  }
  String str(value);
  Serial.println(str);
  str.trim();
  return str;
}



/*
* Name: sendToWifi
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendToWifi(String command, const int timeout, boolean debug){
  String response = "";
  Serial.print("Lähetetään WiFiSerialille: ");
  Serial.println(command);
  wifiSerial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(wifiSerial.available())
    {
    // The esp has data so display its output to the serial window 
    char c = wifiSerial.read(); // read the next character.
    response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}

/*
* Name: sendToUno
* Description: Function used to send data to Arduino.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendToUno(String command, const int timeout, boolean debug){
  String response = "";
  Serial.println(command); // send the read character to the esp8266
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

int calculateTime (int giventime)
{
  Serial.println("Calculate time");
  int totalTime = giventime;
  currentTime = millis();
  partTime = totalTime / 100 * 80;  //partTime is 80% of given time 


}


/************************
* Name: openLock
* Description: Funtion send pulse to magneticlock and open it
* Returns: -
*************************/

int openLock(int lock){
  Serial.println("openLock");
  if (lock == 1){
    Serial.println("lock1");
    digitalWrite(highLowPin, HIGH); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin, LOW);  // sets power off to relay
    delay(2000);
  }else if (lock == 2){
    Serial.println("lock2");
    digitalWrite(highLowPin2, HIGH); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin2, LOW);  // sets power off to relay
    delay(2000);
    }
 
}
 
 
