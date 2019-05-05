#include <AltSoftSerial.h>

// TMP36 parameters
int tempSensorPin = A0;  // Temp sensor in pin0

// Lock parameters
int highLowPin = 12; //Pin to set High and 
int highLowPin2 = 14;
int lock = 1;
int lock2 = 2;

bool DEBUG = true;   //show more logs
int responseTime = 100; //communication timeout
#define serialBaudrate 115200  // Baudrate for Arduino serial bus (PC<->Arduino)
#define wifiSerialBaudrate 9600 // Baudrate for wifiSerial (Arduino<->ESP8266 wifi module)

int time1 = 4;
int time2 = 6;
int time3 = 8;
int time4 = 10;
int time5 = 12;


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
  //readTemperature();
  if(Serial.available()>0){
    String message = readSerialMessage();
    Serial.print("USB serialilta: ");
    Serial.println(message);
    if(find(message,"debugEsp8266:")){
      Serial.println("Lähetetään WiFi serialille");
      String result = sendToWifi(message.substring(13,message.length()),responseTime,DEBUG);
      Serial.print("Response: ");
      Serial.println(result);
      if(find(result,"OK"))
        sendData("\nOK");
        
      else
        sendData("\nEr");
    }
  }
  if(wifiSerial.available()>0){
    
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
    }else
    if(find(message,"HELLO")){  //receives HELLO from wifi
        Serial.println("HI!");    //arduino says HI
        sendData("HI!");
    }else if(find(message,"TEMP")){
      Serial.println("TEMP:22");    //arduino sends temperature
      sendData("TEMP:22");
    }else
    if(find(message,"time1")){  //receives timevalue from wifi
      calculateTime(time1);    
    }else if(find(message,"time2")){ //receives timevalue from wifi
      calculateTime(time2);
    }else if(find(message,"time3")){ //receives timevalue from wifi
      calculateTime(time3);
    }else if(find(message,"time4")){ //receives timevalue from wifi
      calculateTime(time4);
    }else if(find(message,"time5")){ //receives timevalue from wifi
      calculateTime(time5);
      }
    else{
      Serial.print("\nUnknown command!");
      Serial.println(message);
      sendData("\nUnknown command!");                 //Command ERROR CODE for UNABLE TO READ 
    }
  }
  delay(responseTime);
}


void readTemperature(){
  /*int adc = analogRead(tempSensorPin);
  // Serial.println(adc);
  int milliVolts = analogRead(tempSensorPin) * (5000/1024);
  double voltage = double(adc) * 5.0 / 1024;
  // Serial.println(voltage);
  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                                //to degrees ((voltage - 500mV) times 100)
  Serial.print(temperatureC);
  Serial.println(" degrees C");*/
  sendData("temperature");
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
* Description: Function calculate 80% of given time, start timer and ask temperature
*
************************/

int calculateTime (int giventime)
{
 int time = giventime;
 int partTime = 0;
 partTime = time / 100 * 80; // partTime is integer because it is enough exact time
 unsigned long waitTime = partTime * 60 * 60* 1000; //changing hours to msecond
 time = (time - partTime)* 60 * 60 * 1000; //calculate remaining time 
 delay(waitTime);
 
 if (temperature()){
    openLock(lock);
    secondTimer();
 }
 else{
    delay(time);
    openLock(lock);
 }
}

/***********************
* Name: temperature
* Description: Function read value from pin A0
* Returns: True or False
*************************/

boolean temperature(){

 //getting the voltage reading from the temperature sensor
/* int readPin = analogRead(sensorPin);
 float temperatureC = 0; //Readed and modified temperature 
 
 // converting that reading to voltage
 float voltage = readPin * 3.3;
 voltage /= 1024.0; 
 
 
 temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                        //to degrees ((voltage - 500mV) times 100)
 delay(1000);                                     //waiting a second
 if (temperatureC <= -20.0){
    return (true);
 }
 else{
    return (false);
 }*/
 return (true);
}

/************************
* Name: openLock
* Description: Funtion send pulse to magneticlock and open it
* Returns: -
*************************/

int openLock(int lock){

  if (lock == 1){
    digitalWrite(highLowPin, HIGH); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin, LOW);  // sets power off to relay
    delay(2000);
  }else if (lock == 2){
    digitalWrite(highLowPin2, HIGH); // sets power on to relay
    delay(2000);                      // waits for two second
    digitalWrite(highLowPin2, LOW);  // sets power off to relay
    delay(2000);
    }
 
}
 
 /***********************
 * Name: secondTimer
 * Description: Function count time for second lock
 * Returns: _
 ************************/
 
 void secondTimer() {
 
    unsigned long waitTime = 2UL * 60 * 60* 1000; //changing hours to second
 
    delay(waitTime);
    openLock(lock2);
 }
