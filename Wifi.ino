#include <AltSoftSerial.h>


int tempSensorPin = A0;  // Temp sensor in pin0

bool DEBUG = true;   //show more logs
int responseTime = 100; //communication timeout
#define serialBaudrate 115200  // Baudrate for Arduino serial bus (PC<->Arduino)
#define wifiSerialBaudrate 9600 // Baudrate for wifiSerial (Arduino<->ESP8266 wifi module)


AltSoftSerial wifiSerial;      //  for ESP8266

void setup()
{
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
        Serial.print("\\nHI!");    //arduino says HI
        sendData("\\nHI!");
    }else if(find(message,"TEMP")){
      Serial.print("\\nTEMP:22");    //arduino sends temperature
      sendData("\\nTEMP:22");
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
