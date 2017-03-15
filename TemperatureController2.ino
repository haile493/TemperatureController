
/*--------------------------------------------------------------
  Program:      Temperature Controller

  Description:  Arduino web server that displays 4 analog inputs,
                the state of 4 values and controls 2 outputs using buttons.
                The web page is stored on the micro SD card.
  
  Hardware:     Arduino Mega 2560 and compatible Arduino Ethernet
                shield ENC28J60. Should work with other Arduinos and
                compatible Ethernet shields.
                8Gb micro SD card formatted FAT32.
                A2 to A4 analog inputs, pins 8 to 9 as outputs (CBs).
                
  Software:     Developed using Arduino 1.6.7 software
                Should be compatible with Arduino 1.0 +
                SD card contains web page called index.htm
  
  References:   - WebServer example by David A. Mellis and 
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD
                - eth_websrv_SD_Ajax_in_out program by W.A. Smith
                  http://startingelectronics.com

  Date:         8 April 2016
   
  Author:       Le Thanh Hai
--------------------------------------------------------------*/

#include <UIPEthernet.h>
#include <SD.h>
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 20); // IP address, may need to change depending on network
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);  // create a server at port 80
EthernetClient client;
File webFile;               // the web page file on the SD card
//char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
//char req_index = 0;              // index into HTTP_req buffer
String readString;
//boolean LED_state[4] = {0}; // stores the states of the LEDs
boolean CB_state[2] = {0}; // stores the states of the LEDs
int temper_val[4] = {0}; // stores the states of the temperature sensors
int day = 8, month = 4, year = 2016, hour = 10, minute = 0, second = 0;
String dates, times, strd1H, strd1L, strd2H, strd2L;
int delta_val[4] = {2, 2, 2, 2}; // delta1 high, delta1 low, delta2 high, delta2 low
int Startpos, Endpos;

void setup()
{
    // disable Ethernet chip
    //pinMode(10, OUTPUT);
    //digitalWrite(10, HIGH);
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
    // switches on pins 2, 3 and 5
    //pinMode(2, INPUT);
    //pinMode(3, INPUT);
    //pinMode(5, INPUT);
    // LEDs
    //pinMode(6, OUTPUT);
    //pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    //pinMode(9, OUTPUT);
    pinMode(13, OUTPUT);
    
    Ethernet.begin(mac, ip, gateway, subnet, 10);  // initialize Ethernet device
    Ethernet.begin(mac, 10);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    
}

void loop()
{
    // read/write data from/to website
    
    
    homepage();
    
}


void homepage(){
  client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                //if (req_index < (REQ_BUF_SZ - 1)) {
                if (readString.length() < (REQ_BUF_SZ - 1)) {
                    //HTTP_req[req_index] = c;          // save HTTP request character
                    //req_index++;
                    readString += c;
                }
                //Serial.println("running");
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    //if (StrContains(HTTP_req, "ajax_inputs")) {
                    if (readString.indexOf("ajax_inputs") > 0) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetIOs();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    //Serial.print(HTTP_req);
                    //Serial.print(readString);
                    
                    // reset buffer index and all buffer elements to 0
                    //req_index = 0;
                    //StrClear(HTTP_req, REQ_BUF_SZ);
                    readString = "";
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

// checks if received HTTP request is switching on/off CBs
// also saves the state of the CBs
void SetIOs(void)
{
    //Serial.println(readString);
    // CB 1 (pin 8)
    //if (StrContains(HTTP_req, "CB1=1")) {
    if (readString.indexOf("CB1=1") > 0) {
        CB_state[0] = 1;  // save CB state
        digitalWrite(8, HIGH);
    }
    //else if (StrContains(HTTP_req, "CB1=0")) {
    if (readString.indexOf("CB1=0") > 0) {
        CB_state[0] = 0;  // save CB state
        digitalWrite(8, LOW);
    }
    // CB 2 (pin 9)
    //if (StrContains(HTTP_req, "CB2=1")) {
    if (readString.indexOf("CB2=1")>0) {
        CB_state[1] = 1;  // save CB state
        //digitalWrite(9, HIGH);
        digitalWrite(13, HIGH);
    }
    //else if (StrContains(HTTP_req, "CB2=0")) {
    if (readString.indexOf("CB2=0") > 0) {
        CB_state[1] = 0;  // save CB state
        //digitalWrite(9, LOW);
        digitalWrite(13, LOW);
    }
    // delta values
    if (readString.indexOf("d1H=") > 0) {        
        //Serial.println(readString);
        Startpos = readString.indexOf("d1H=") + 4;
        Endpos = readString.indexOf("&d1L");
        strd1H = readString.substring(Startpos, Endpos);
        //Serial.println(readString);
        delta_val[0] = strd1H.toInt();        
    }
    if (readString.indexOf("d1L=") > 0) {        
        Startpos = readString.indexOf("d1L=") + 4;
        Endpos = readString.indexOf("&d2H");
        strd1L = readString.substring(Startpos, Endpos);        
        delta_val[1] = strd1L.toInt();        
    }
    if (readString.indexOf("d2H=") > 0) {        
        Startpos = readString.indexOf("d2H=") + 4;
        Endpos = readString.indexOf("&d2L");
        strd2H = readString.substring(Startpos, Endpos);        
        delta_val[2] = strd2H.toInt();        
    }
    if (readString.indexOf("d2L=") > 0) {        
        Startpos = readString.indexOf("d2L=") + 4;
        Endpos = readString.indexOf("&nocache");
        strd2L = readString.substring(Startpos, Endpos);        
        delta_val[3] = strd2L.toInt();        
    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    //int analog_val;            // stores value read from analog inputs
    int count;                 // used by 'for' loops
    //int sw_arr[] = {2, 3, 5};  // pins interfaced to switches
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");

    // get date and time
    dates= String(day) + "/" + String(month) + "/" + String(year) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
    //times =  String(hour) + ":" + String(minute) + ":" + String(second);
    cl.print("<datesensor>");    
    cl.print(dates);    
    cl.println("</datesensor>");    
    
    // read analog inputs
    for (count = 0; count <= 3; count++) { // A2 to A5
        temper_val[count] = analogRead(count+2);
        cl.print("<analog>");
        cl.print(temper_val[count]);        
        cl.println("</analog>");
    }

    // read high and low thresholding values for controlling bump
    for (count = 0; count <= 3; count++) {        
        cl.print("<deltaval>");
        cl.print(delta_val[count]);        
        cl.println("</deltaval>");
    }
    
    // button CB states
    // CB1
    cl.print("<CB>");
    if (CB_state[0]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</CB>");
    // CB2
    cl.print("<CB>");
    if (CB_state[1]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</CB>");
    
    cl.print("</inputs>");
}
/*
// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}*/
