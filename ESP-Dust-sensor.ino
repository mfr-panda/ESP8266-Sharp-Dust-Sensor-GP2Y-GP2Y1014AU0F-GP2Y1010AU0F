/* 
Sketch by Panda
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>

// includes necessaires au fonctionnement de l'OTA :
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

AsyncWebServer server(80);

#include <GP2YDustSensor-PandaCustom.h>  // Version modifiée de la librairie GP2YDustSensor

const uint8_t SHARP_LED_PIN = 4;   // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = A0;    // Sharp Dust/particle analog out pin used for reading 
const uint16_t AVERAGE_SAMPLES = 120;

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, SHARP_LED_PIN, SHARP_VO_PIN, AVERAGE_SAMPLES);

int resultDust = 0;
int dustAverage = 0;
float newBaseline = 0.6;
char logBuffer[255];
int count = 0;
static int errorRaw = 5; //seuil erreur Raw capteur
boolean sensorState = true;
boolean setOffset = false;

/*Router SSID & Password*/
char* ssid_1 = "SSID-1";                       // Identifiant WiFi
char* password_1 = "PASSWORD-1";                // Mot de passe WiFi

char* ssid_2 = "SSID-2";                       
char* password_2 = "PASSWORD-2";

char* ssid = "";
char* password= "";

char* ssid_ap = "sonde-particules-fablac";

boolean ssid_ok = false;
boolean ssid_con = false;

String AdrIP;
String AdrIP_AP;

/* ****************************
 *  SCAN WIFI
 *  **************************/

void scanWifi() {
  int n = WiFi.scanNetworks();
  Serial.println("scan OK");  
  if (n == 0) {
    Serial.println("Pas de réseaux");    
  } else {
    Serial.print(n);
    Serial.println(" Réseaux trouvés");
    
    for (int i = 0; i < n; ++i) {
      // Print SSID pour chaque réseau
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.println(WiFi.RSSI(i));
            
      if(WiFi.SSID(i) == ssid_1){ 
      Serial.println(String(ssid_1) + " : Réseau Connu");
      ssid = ssid_1;
      password = password_1;
      ssid_ok=true;
      }
      if(WiFi.SSID(i) == ssid_2){ 
      Serial.println(String(ssid_2) + " : Réseau Connu");
      ssid = ssid_2;
      password = password_2;
      ssid_ok=true;
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////

void setup(){
    Serial.begin(115200);

    int i=0;
    scanWifi();
    delay(100);
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid_ap, NULL,7,0,5);
    delay(2000);   

    if (ssid_ok == true){
      Serial.println("Connecting to ");
      Serial.println(ssid);    
    //connexion au réseau wifi local
      WiFi.begin(ssid, password);
  
      while ((WiFi.status() != WL_CONNECTED) and (i<30)) {
      delay(500);
      i=i+1;      
      Serial.print(".");
      }
      if (i<29) {
        Serial.println("");
        Serial.println("WiFi connected..!");
        Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
        AdrIP = WiFi.localIP().toString();
        ssid_con = true; 
      }  

      }
      if (MDNS.begin("Sonde-Particules-FabLac")) {
        Serial.println("MDNS responder started : Sonde-Particules-FabLac");
      delay(500);      
  }

  server.on("/", HTTP_GET, handle_OnConnect);
  server.onNotFound(notFound);

  server.begin();
  Serial.println("HTTP server started");

  ArduinoOTA.setHostname("SondeParticule"); // on donne une petit nom a notre module
  ArduinoOTA.setPassword("F4bL4c");
  ArduinoOTA.begin(); // initialisation de l'OTA

  count = 0 ;
  dustSensor.begin();  

  delay(2000);  
}

/////////////////////////////////////////////////////////////////////////////

void handle_OnConnect(AsyncWebServerRequest *request) {
  request->send(200, "text/html", SendHTML(resultDust,dustAverage,newBaseline)); 
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

String SendHTML(float resultDust, float dustAverage, float newBaseline){
  
  String ptr = "<!doctype html><head><meta charset=\"UTF-8\">\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<meta http-equiv=\"refresh\" content=\"2\">\n";
  ptr +="<title>Sonde Particules FabLac</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Sonde Particules FabLac</h1>\n";
  ptr +="<p style=\"text-decoration:underline;\">Densité Particules :</p><p>Valeur instantanée<span style=\"font-weight:bold;\"><br>";
  if (sensorState == true){
    ptr +=resultDust;
    ptr +=" ug/m3</span></p>";
  } else {
    ptr += "Erreur Capteur</span></p>";
  } 
  ptr += "<p>Moyenne ( ";
  ptr +=AVERAGE_SAMPLES;
  ptr +=" val) ";
  ptr +=dustAverage;
  ptr += " ug/m3</p>";
  ptr +="<p>Offset ";
  if (setOffset == true){
    ptr +="(mise à jour..) "; 
    } 
  ptr +=newBaseline;
  ptr +=" V</p>";
  ptr +="</div>\n";
  ptr +="<p style=\"font-size:0.9em;\">[ IP AP Local : 192.168.4.1 ]";  
  if (ssid_con == true){
    ptr +="<br>[ connecté à : ";
    ptr +=ssid;
    ptr +=" ]<br>[ IP : ";
    ptr +=AdrIP; 
    ptr +="";     
    }
  ptr +=" ]</p>"; 
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

/////////////////////////////////////////////////////////////////////////////

void loop(){

  ArduinoOTA.handle();

  int RawVal = analogRead(SHARP_VO_PIN);  // si valeur A0 < 5 probleme de capteur
  Serial.print("Valeur Raw : ");
  Serial.println(RawVal);
  if (RawVal < errorRaw){
    sensorState = false;     
  } else {
    sensorState=true;  
  }
   
  resultDust = dustSensor.getDustDensity();
  dustAverage = dustSensor.getRunningAverage();
  sprintf(logBuffer, "Densité: %d ug/m3; Moyenne: %d ug/m3", resultDust, dustAverage);
  Serial.println(logBuffer);
  
  count = count +1;
 
// Mise a jour de l'offset
  if (count > 20) {
    setOffset = true;
    for (int i=0; i <= 120; i++) {
      ArduinoOTA.handle();
      resultDust = dustSensor.getDustDensity();
      sprintf(logBuffer, "Density: %d ug/m3", resultDust);
      Serial.println(logBuffer);
      delay(580);
    }

    dustAverage = dustSensor.getRunningAverage();
    newBaseline = dustSensor.getBaselineCandidate();
  
    sprintf(logBuffer, "1m Avg Dust Density: %d ug/m3; New baseline: %.4f", dustAverage, newBaseline);
     Serial.println(logBuffer);

    // compensates sensor drift after 1m
    dustSensor.setBaseline(newBaseline);

    count = 0;  
  }
  setOffset = false;
  
  delay(3000);

}
