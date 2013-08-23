
/*
 * Listen for commands to trip relays for a enternet shield equiped arduino
 * 
 * Some is from the Arduino Ethernet
 * WebServer example and some from Limor Fried (Adafruit) so its probably under GPL
 *
 * Tutorial is at http://www.ladyada.net/learn/arduino/ethfiles.html
  */

#include <Ethernet.h>
#include <SPI.h>

/************ ETHERNET STUFF   ************/
// No reserved IP at home, hope it's OK
// MAC 90-A2-DA-00-47-C6
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x47, 0xC6 };
byte ip[] = { 10, 0, 0, 153 };
EthernetServer server(80);

/************ light stuff **************/
int numlights=4; // number of elements of the arrays below
int numopts=2; // we have 2 options:
char* lopts[]={"5-min Warning","Come up NOW!"}; // 5-min warning, Come up NOW!
boolean optstates[]={1,1}; // We set this via form and take action accordingly? Or I won't use it?
boolean states[]={1,1,1,0}; // list of the light/relay states
boolean blinkstates[]={0,1,0,0};  // list of which lights/relays should be in the blink cycle, limited to yellow-only now
int pins[]={14,15,16,17}; // list of the light/relay pins
char* lights[]={"red","yellow","green","beacon"}; // list of the "light" lables
int blinkc=1;
int blinkmax=20000;

/*********** light/relay routines ****************/

void setlights() { // turn the lights on or off
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    setone(states[i],pins[i]);
  }
}

void fivemin() { // progressively turn on green/yellow/red

}

void blinkem() { // blink the lights fn
  if (blinkc == 1 ) { // turn on
    Serial.println("On");
    setstates(true);
//    states[3]=false; // don't flash on the beacon, it's obnoxious.
    setlights();
    blinkc++;
  } else if (blinkc == (blinkmax/2)) { // half way through, turn off
    Serial.println("Off");
    setstates(false);
    setlights();
  } else if (blinkc > blinkmax) { // reset loop
    blinkc=1;
  }
  if (blinkc >1 ) { // increment loop if counter is set
    blinkc++;
  }
}

void upnow() { // blink the lights fast!
  blinkmax=10000;
  blinkstates[0]=1;
  blinkstates[1]=1;
  blinkstates[2]=1;
  blinkstates[3]=0;
  blinkem();
}

void getlights() { // turn the lights on or off
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    states[i]=digitalRead(pins[i]);
  }
}

void setone(boolean light, int pin) { //turn on one light
  if (light) {
    digitalWrite(pin,HIGH);
    Serial.print("turning on ");
    Serial.println(pin,DEC);
  } else {
    digitalWrite(pin,LOW);
  }
}

void setstates(boolean newstate) { // set all the states to be on or off, used in blinking
  for (int i = 0; i < numlights; i++) { // loop through all the lights...
    states[i]=newstate && blinkstates[i]; // only turn on lights listed in "blinkstates"
  }
}

void doform(EthernetClient client) {  // draw the form
  client.println("<form action='b' action='get'>");
  for (int i = 0; i < numopts; i++) { // loop through all the lights
    dobox(client,lopts[i],states[i]);
  }
  client.println("<p>");
  client.println("<input type='submit' value='Submit' name='submit'>"); // we don't need the closing </input>?
  client.println("</p>");
  client.println("</form>");
}

void dobox(EthernetClient client,char item[], boolean checked) {  // draw a single checkbox
  client.println("<p>");
  client.print("<input type='checkbox' name='c' value='");
  client.print(item);
  client.print("'");
  if (checked) {
    client.print(" checked");
  }
  client.print(">");
  client.print(item);
  client.println("<br>");
  client.println("</p>");
}

// How big our line buffer should be. 100 is plenty!
#define BUFSIZ 100

void setup() {
  Serial.begin(9600);
 
  // Serial.println("Free RAM: ");
  // Serial.println(FreeRam());  
  
  // set pins as output and turn on all the lights to show that we don't have access yet.
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    pinMode(pins[i], OUTPUT);
  }
  setlights();
  
  pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH);                    // but turn off the W5100 chip!
  // Debugging complete, we start the server!
  Ethernet.begin(mac, ip);
  server.begin();
}


void loop()
{
  char clientline[BUFSIZ];
  int index = 0;
  
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    
    // reset the input buffer
    index = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
          
          // continue to read more data!
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        
        // Print it out for debugging
        Serial.println(clientline);
        
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          doform(client);
          // client.print("Free Ram: ");
          // client.println(FreeRam());        
        } else if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a request!
          char *request;
          request = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
          // print the requested string
          Serial.println(request);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          for (int i = 0; i < numlights; i++) { // loop through all the lights...
            states[i]=false;  // ...and assume they are off
          }
          for (int i = 0; i < numlights; i++) { // loop through all the lights...
            states[i]=false;  // ...and assume they are off
            if (strstr(request, lights[i]) != 0){ // if they asked for this light
              client.print("Turning ");
              client.print(lights[i]);
              client.println(" on<br>");
              states[i]=true;
            }  // end of "asked for this light"     
          }   // end of light loop

          setlights();
          blinkc=0; // stop blinking
          /*  if "error" is sent as part of the request, ignore everything and start the error blink */
          if (strstr(request, "error") != 0) {
            client.println("<h1>The request reported an error:<br>'");
            client.print(request);
            client.println("'</h1>");
            blinkc=1; // restart the blinking
            for (int i = 0; i < numlights; i++) { // loop through all the lights...
              blinkstates[i]=false;  // ...and assume they are off
            }
            if (strstr(request, "error=1") != 0) { // if they reported an error state 1
              blinkstates[1]=1; //yellow on
            }
            if (strstr(request, "error=2") != 0) { // if they reported an error state 2
              blinkstates[0]=1; //red on
            }
            if (!(blinkstates[0] || blinkstates[1])) { // if they reported an error that wasn't listed above
              blinkstates[0]=1; //red on
              blinkstates[1]=1; // yellow on
              blinkstates[2]=1; // green off
            }
            blinkmax=10000; // blink twice as fast
          }

          getlights(); // get the current settings before...
          doform(client); // ... drawing the form
        } else {
          // everything else is a 404
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>HUH?</h2>");
        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
/*******************************************
 *  when first turned on, blink the lights
 *    once blinkc is set to 0, it won't blink any more.
 ******************************************/
  blinkem();
}

