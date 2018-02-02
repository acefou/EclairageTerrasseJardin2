//Pilotage Relais via requete http
// ../ActionPoussoirJardin
// si ok, repond : Envoi impulsion OK 
// temoin sur la BuildInLed
//  ../bonjour
//    renvoi hello
//  ../status
//    renvoi l'ID de l'esp  +  la version du croquis + le compteur d'impulsion realisée
//
//
// Acefou
// V1 28/01/2018


#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RemoteDebug.h>

const char* version_croquis = "acefou v1.0";
const char* ssid = "xxxx";
const char* password = "xxxx";

RemoteDebug Debug;
 
const int PinRelais = D1; // D1 GPIO5
const long interval = 500;
ESP8266WebServer  server(80);
String localWebPage = "";
String localWebPageJardin = "";
String htmlStyle="";
String sHID;

int i=0;
char ID_monESP[ 16 ];

int iCptImpulsion =0;

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
  Debug.println("reception d une requete inconnue");
}

void executeActionPoussoirJardin(){
        digitalWrite(LED_BUILTIN, LOW);     // Turn la LED on
        digitalWrite(PinRelais, HIGH);      // maj GPIO : commande relais ON
        delay(interval);
        digitalWrite(LED_BUILTIN, HIGH);    // Turn la LED off
        digitalWrite(PinRelais, LOW);       // maj GPIO : commande relais OFF
        delay(interval);
        iCptImpulsion +=1;
 }

void setup() {
  // Ouverture du port serie en 115200 baud pour envoyer des messages de debug à l'ide par exemple
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println(version_croquis);

  htmlStyle += "<head><style>h1 {font-size: 56px;text-align: center;text-transform: uppercase;color: #3366ff;} .button{";
  htmlStyle += "display: inline-block;padding: 15px 25px;font-size: 96px;cursor: pointer;text-align: center;text-decoration: none;outline: none;color: #fff;";
  htmlStyle += "background-color: #4CAF50;border: none;border-radius: 15px;box-shadow: 0 9px #999;width: 100%;}";
  htmlStyle += ".button:hover {background-color: #3e8e41}";
  htmlStyle += ".button:active {background-color: #3e8e41;box-shadow: 0 5px #666;transform: translateY(4px);}</style></head>";

  localWebPage += htmlStyle;
  localWebPage += "<p><br></p><h1> Commande Eclairage Jardin </h1>";
  localWebPage += "<div style='text-align: center;'>";
  localWebPage += "<p><a href=\"ActionPoussoirJardin\">";
  localWebPage += "<button class='button'>On / Off</button></a></p></div>";
  localWebPage += "<p><br>propuls&eacute; par Acefou</p>";


  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED off
  // prepare GPIO5 (sortie)
  pinMode(PinRelais, OUTPUT);
  digitalWrite(PinRelais, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connection à ");
  Serial.println(ssid);

  // On se connecte a reseau WiFi avec le SSID et le mot de passe precedemment configure
  WiFi.begin(ssid, password);

  // On sort de la boucle uniquement lorsque la connexion a ete etablie. Sinon, on reboot le module ESP
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {
    delay(500);
    Serial.print(".");
    if (i==21){
      Serial.println("Echec Connection WiFi ! Rebooting...");
      delay(5000);
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connecté");

  // connexion OK, on demarre le server web // Start the server
  server.begin();
  Serial.println("Server démarré");

  // On indique sur le port serie l'adresse ip de l'ESP pour le trouver facilement
  Serial.println(WiFi.localIP());

  uint32_t n = ESP.getChipId();
  snprintf(ID_monESP, sizeof ID_monESP, "%08X\n", (unsigned long)n); 
  // == Serial.printf("Chip ID = %08X\n", ESP.getChipId());
  //Serial.println(ID_monESP);
  //Serial.println("");

// initialisation du debug telnet
  Debug.begin(ID_monESP);

// Routage
  //------route racine
    server.on("/", [](){
      server.send(200, "text/html", localWebPage);
      Debug.println("reception d une requete home");
      });
  //-----route /bonjour 
    server.on("/bonjour", [](){
      server.send(200, "text/plain", "hello !");
      Debug.println("reception d une requete bonjour");
      });
  //-----route /ActionPoussoirJardin 
    server.on("/ActionPoussoirJardin", []() {
        executeActionPoussoirJardin();
        //page de retour 200
        server.send(200, "text/html", localWebPage);
        Debug.println("reception d une requete ActionPoussoirJardin");
        });
  //-----route /status  
  server.on("/status", [](){
         //server.send(200, "text/plain", "status ok");
         String s = "<!DOCTYPE HTML>\r\n<html><p>";
         s += version_croquis;
         s += "</p><p>ID_monESP:";
         s += sHID;
         s += "</p><p>Nb de commande envoy&eacute;es: ";
         s += iCptImpulsion;
         s += "</p><p>READY et OK.</p>";
         s += "<a href='../'>Retour</a>";
         s += "</html>\n";
         server.send(200, "text/html", s);
         Debug.println("reception d une requete Status");
        });  

  server.onNotFound ( handleNotFound );
        
  // on commence a ecouter les requetes venant de l'exterieur
  server.begin();

  ArduinoOTA.onStart([]() {
    Debug.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Debug.println("End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Debug.println(".");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Debug.println("Error");
    if (error == OTA_AUTH_ERROR) Debug.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Debug.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Debug.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Debug.println("Receive Failed");
    else if (error == OTA_END_ERROR) Debug.println("End Failed");
  });

  //construit le nom du host à partir de l'ID unique de ESP
  int iID = ESP.getChipId();
  sHID = "ESP_ID_" ;
  sHID +=String(iID, HEX); 
  char charBuf[50];
  sHID.toCharArray(charBuf, 50) ;

  ArduinoOTA.setHostname(charBuf); 
  Serial.println(charBuf);
  ArduinoOTA.begin(); 



//signature de la fin du setup avec la led built-in : 3 flash de 2 secondes
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);   
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);   
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);   
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("READY : GO to Loop");
}

void loop() {
      ArduinoOTA.handle();   
      Debug.handle();
      server.handleClient();
}
