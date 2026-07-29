#include <PID_v1.h> // PID library by Brett Beauregard
#include "WiFiS3.h" // wifi lib
#include <Servo.h> //Builtin servo library
#include "page.h" // page UI

////////////////////////////////
//                            //
//    Networking Variables    //
//                            //
////////////////////////////////

char ssid[] = "wheelchairbrake";
char pass[] = "password";   
int keyIndex = 0; 

int status = WL_IDLE_STATUS;
WiFiServer server(80);

////////////////////////////////
//                            //
//      Servo Variables       //
//                            //
////////////////////////////////

Servo motor;

const int pinServo = 9; //Position of servo's info pin
double servoPos = 0; //Initialize the servo's initial position
int servoWriteDelay = 15;

////////////////////////////////
//                            //
// Velocity Sensor Variables  //
//                            //
////////////////////////////////

const int pinHallEffect = 2; //Hall effect sensor's pin, code will automatically run when this voltage changes to get accurate counts

double velocityRot = 0; //Wheel's rotational velocity in rad/s
double velocityTrans = 0; //PID's input; Translational velocity at this current instant in m/s.

const float diameterWheel = 0.05; //Wheel's diameter (m)

volatile unsigned long lastPulseTime = 0; // most recent HE detection
volatile unsigned long period = 0; // Time between the last two HE detections
const unsigned int TIMEOUT = 2000000; // max amount of time the wheel doesn't revolve (2 seconds)


////////////////////////////////
//                            //
//        PID Variables       //
//                            //
////////////////////////////////

//double velocityTransServo = 0; //Not the actual device's translational velocity, but the value read by the servo.
double velocityTransTarget = 0.1; //PID's setpoint; Max speed that the PID controller is trying to reach in m/s

double Kp=25, Ki=5, Kd=25; //PID parameters; Proportion, integral, and derivative as scalars
PID myPID(&velocityTrans, &servoPos, &velocityTransTarget, Kp, Ki, Kd, DIRECT); //Initialize the PID controller

void setup() {
  ////////////////////////////////
  //                            //
  //        Servo Setup         //
  //                            //
  ////////////////////////////////
  
  motor.attach(pinServo); //Initializes the servo's pin
  motor.write(0); //Forces the servo to the minimum position (THIS MIGHT BE REDUNDANT PLEASE VERIFY)

  ////////////////////////////////
  //                            //
  //   Velocity Sensor Setup    //
  //                            //
  ////////////////////////////////
  
  pinMode(pinHallEffect, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinHallEffect), checkPulse, FALLING); // interrupt process to check pulse, runs whenever voltage falls in pin2

  ////////////////////////////////
  //                            //
  //    PID Controller Setup    //
  //                            //
  ////////////////////////////////

  myPID.SetMode(AUTOMATIC); //Turns the PID controller on

  ////////////////////////////////
  //                            //
  //      Networking Setup      //
  //                            //
  ////////////////////////////////

  // Access Point setup
  Serial.begin(9600); // initialize serial comm
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("Access Point Web Server");

  // Check for module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Check firmware
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Config IP
  WiFi.config(IPAddress(192,48,56,2));

  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Start access point
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // Begin server
  delay(10000);
  server.begin();

  printWiFiStatus();

}

void loop() {

  ////////////////////////////////
  //                            //
  //      Networking Loop       //
  //                            //
  ////////////////////////////////

  // Check whether WiFi connection status changed
  if (status != WiFi.status()) {
    status = WiFi.status();

    delay(5000);

    if (status == WL_AP_CONNECTED) {
      Serial.println("Device connected to AP");
    } else {
      Serial.println("Device disconnected from AP");
    }
  }

  // Listen for an HTTP client
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client");

    String currentLine = "";
    String reqLine = "";        // holds get request
    unsigned long start = millis();

    while (client.connected()) {
      // don't hang forever on a phone that opens a socket and goes quiet
      if (millis() - start > 2000) break;

      if (client.available()) {
        char c = client.read();

        if (c == '\n') {
          // Blank line means the HTTP request has ended
          if (currentLine.length() == 0) {
            handleRequest(client, reqLine);
            break;
          } else {
            // first line of the request is the one with the route in it
            if (reqLine.length() == 0) reqLine = currentLine;
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    Serial.println("Client disconnected");
  }

  ////////////////////////////////
  //                            //
  //         Servo Loop         //
  //                            //
  ////////////////////////////////

  //This might be redundant because of the PID controller loop

  ////////////////////////////////
  //                            //
  //    Velocity Sensor Loop    //
  //                            //
  ////////////////////////////////

  velocityTrans = getSpeed();

  // test print
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print(velocityTrans);
    Serial.print(" m/s     ");
    Serial.print((velocityTrans * 3600) / 1609.34); // conv to mph
    Serial.print(" mph     Target speed: ");
    Serial.print((velocityTransTarget * 3600) / 1609.34);
    Serial.print("\t ServoPos =");
    Serial.println(servoPos);
  }

  ////////////////////////////////
  //                            //
  //    PID Controller Loop     //
  //                            //
  ////////////////////////////////

  if (velocityTrans > velocityTransTarget)
  {
    myPID.Compute(); //Compares the current translational velocity to the target maximum and queues a new servo rotation accordingly
  }
  else
  {
    servoPos = 0;
  }
  analogWrite(pinServo, servoPos); //Send new servo rotation to servo
  //This functionality might need a delay without pause (DO NOT USE delay() because it pauses the whole program)

}

  ////////////////////////////////
  //                            //
  //          Routing           //
  //                            //
  ////////////////////////////////

void handleRequest(WiFiClient &client, String req) {
  Serial.print("REQ: ");
  Serial.println(req);

  // set speed from get req
  if (req.startsWith("GET /set")) {
    int i = req.indexOf("v=");
    if (i >= 0) {
      int v = req.substring(i + 2).toInt();
      velocityTransTarget = (v * 1609.34) / 3600; // conv to m/s
      Serial.print("target speed = ");
      Serial.println(velocityTransTarget);
    }
    sendPlain(client, String(velocityTransTarget));

  } else if (req.startsWith("GET /state")) {
    sendPlain(client, String(velocityTransTarget));

  } else {
    sendPage(client);
  }
}

////////////////////////////////
//                            //
//         HE Helpers         //
//                            //
////////////////////////////////

// set pulse variables
void checkPulse(){
  unsigned long currentTime = micros();
  unsigned long downTime = currentTime - lastPulseTime;

  if (downTime < 2000){ // false pulse if quicker than 2ms
    return;
  }
  lastPulseTime = currentTime;
  period = downTime;
}

// Estimates the current speed AT each sensor detection rather than calculating speed every second, for quicker updates

float getSpeed(){

  if (period == 0 || micros() - lastPulseTime > TIMEOUT) { // no input yet, or wheel stopped
    return 0;
  }

  float turnsPerSecond = 1000000.0 / (float)period; // estimates turns per second based on how long the last rotation took
  return turnsPerSecond * diameterWheel * 3.14159; // m/s
}

// display original page
void sendPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.print("Content-Length: ");
  client.println(strlen(PAGE_HTML));
  client.println("Connection: close");
  client.println();
  client.print(PAGE_HTML);
}

// display updated page
void sendPlain(WiFiClient &client, String body) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println("Connection: close");
  client.println();
  client.print(body);
}

void printWiFiStatus() {
  // print the SSID of the network
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print ip address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("Open browser at -> http://");
  Serial.println(ip);

}


