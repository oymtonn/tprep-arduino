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

// control state
const int SPEED_MIN = 0;
const int SPEED_MAX = 5;
int targetSpeed = 1;   // what the slider is ini set to

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

const int pinHallEffect = 8; //Hall effect sensor's pin

double velocityRot = 0; //Wheel's rotational velocity in rad/s
double velocityTrans = 0; //PID's input; Translational velocity at this current instant in m/s.

const float diameterWheel = 50; //Wheel's diameter
volatile unsigned int pulseCount = 0; //The amount of times the hall effect sensor detects an input

////////////////////////////////
//                            //
//        PID Variables       //
//                            //
////////////////////////////////

double velocityTransMax = 1; //PID's setpoint; Max speed that the PID controller is trying to reach in m/s

double Kp=2, Ki=5, Kd=1; //PID parameters; Proportion, integral, and derivative as scalars
PID myPID(&velocityTrans, &servoPos, &velocityTransMax, Kp, Ki, Kd, DIRECT); //Initialize the PID controller

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
  
  //For the hall effect sensor

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

  //Read the wheel's pulse count and elapsed time
  //Convert read data to rotational velocity
  velocityTrans = velocityRot * (diameterWheel / 2);//Convert rotational velocity to translational velocity in m/s

  ////////////////////////////////
  //                            //
  //    PID Controller Loop     //
  //                            //
  ////////////////////////////////

  myPID.Compute(); //Compares the current translational velocity to the target maximum and queues a new servo rotation accordingly
  analogWrite(pinServo, servoPos); //Send new servo rotation to servo
  //This functionality might need a delay without pause (DO NOT USE delay() because it pauses the whole program)

}

// routing

void handleRequest(WiFiClient &client, String req) {
  Serial.print("REQ: ");
  Serial.println(req);

  // set speed from get req
  if (req.startsWith("GET /set")) {
    int i = req.indexOf("v=");
    if (i >= 0) {
      int v = req.substring(i + 2).toInt();
      targetSpeed = constrain(v, SPEED_MIN, SPEED_MAX); // speed must be between 0-5
      Serial.print("target speed = ");
      Serial.println(targetSpeed);
    }
    sendPlain(client, String(targetSpeed));

  } else if (req.startsWith("GET /state")) {
    sendPlain(client, String(targetSpeed));

  } else {
    sendPage(client);
  }
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


