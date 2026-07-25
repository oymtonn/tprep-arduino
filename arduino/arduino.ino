#include <PID_v1.h> // PID library by Brett Beauregard
#include "WiFiS3.h" // wifi lib

// define variables
double setpoint, input, output;

char ssid[] = "wheelchairbrake";
char pass[] = "wheelchairbrake111";   
int keyIndex = 0; 

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// ui components on arduino client

void setup() {
  // initialize PID variables


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

  // PID setup

}

void loop() {
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

    while (client.connected()) {
      delayMicroseconds(10);

      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          // Blank line means the HTTP request has ended
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<body>");
            client.println(
              "<p style=\"font-size:7vw;\">"
              "Click <a href=\"/H\">here</a> to turn the LED on"
              "</p>"
            );
            client.println(
              "<p style=\"font-size:7vw;\">"
              "Click <a href=\"/L\">here</a> to turn the LED off"
              "</p>"
            );
            client.println("</body>");
            client.println("</html>");

            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // do things, based on get request
        // if (currentLine.endsWith("GET /H")) { ... }
        // if (currentLine.endsWith("GET /L")) { ... }
      }
    }

    client.stop();
    Serial.println("Client disconnected");
  }

  // PID Controls
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


