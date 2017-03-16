#include <Wire.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
//#include <Ethernet.h>
#include <SD.h>

File cfile;
//EthernetServer server(80);                            // porta servidor

struct NetworkConfig {
  byte mac[6] = { 0x90, 0xA2, 0xDA, 0x00, 0x9B, 0x36 };
  byte dns_srv[4]  {8,8,8, 8};
  byte ip[4] = {10, 10, 1, 88};
  byte gateway[4] = {10, 10, 1, 254};
  byte mask[4] = {255, 255, 255, 0};  
};

struct WifiConfig {
  String netname;      //  your network SSID (name) 
  String netpwd;   // your network password
  int keyIndex = 0;                 // your network key Index number (needed only for WEP)
};

struct ConfigManager {
  int minLimit = 10;
  int maxLimit = 90;
  NetworkConfig network;
  WifiConfig wifi;
};

LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3, POSITIVE);

// define o tempo em milesegundos entre os frames
int x = 300;

// define os simbolos da bomda e agua
byte bomba_e[8] = {B00001,B00010,B11100,B11100,B11100,B11100,B00010,B00001};
byte bomba_d[8] = {B10000,B01000,B00111,B00111,B00111,B00111,B01000,B10000};
byte eixo_p1[8] = {B00000,B10000,B01000,B00100,B00010,B00001,B00000,B00000};
byte agua_e1[8] = {B00000,B00000,B10000,B00000,B00000,B10000,B00000,B00000};
byte agua_e2[8] = {B00000,B00000,B10100,B00000,B00000,B10100,B00000,B00000};
byte agua_e3[8] = {B00000,B00000,B10101,B00000,B00000,B10101,B00000,B00000};

#define trig 2
#define echo 3
#define rele 4

int nivel, bomba_estado = 0;

//int pump_on = 0;
int disable = 0;
int system_mode = 0;
int old_mode = -1;
boolean log_enviado = false;

//EthernetClient client_log;
char log_srv[] = "agua.silvamedina.com.br";

ConfigManager confs;

void setup() {
  // initialize Ethernet port
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(rele, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Inicializando");
  lcd.setCursor(4,1);
  lcd.print("Sistemas");

  delay(2000);
  lcd.clear();
  if(loadConfigs()){
    lcd.setCursor(10,0);
    lcd.print(confs.network.ip[3]);
   
    return;
    //WiFi.config(confs.network.ip, confs.network.dns_srv, confs.network.gateway, confs.network.mask);  
    // initialize Wifi using SSID and Password from SD Config File
    //WiFi.begin(confs.wifi.netname, confs.wifi.netpwd);
  }

  delay(2000);
   // crias so simbolos da bomda e agua
  lcd.createChar(0, bomba_e);  
  lcd.createChar(1, bomba_d);  
  lcd.createChar(2, agua_e1);
  lcd.createChar(3, agua_e2);
  lcd.createChar(4, agua_e3);
  lcd.createChar(5, eixo_p1);

  
   //   limpa o display de LCD
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Nivel  |  Bomba"); 
  lcd.setCursor(10,1);
  pump_off(12,1);
  // start webserver for clients connection
  //server.begin();
}

void loop() {
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

//  pump_on = analogRead(A0);
  disable = analogRead(A1);
  
  int tempo = pulseIn(echo, HIGH);
  int distancia = (tempo/58);
  nivel = map(distancia,90,10,0,100);
  old_mode = system_mode;

  return;
  if (disable >= 100 && pump_on != 0){
    lcd.setCursor(10,1);
    pump_disable(12,1);
    system_mode = 0; // OFF
    bomba_estado = 0;
    if(old_mode != system_mode)
      sendLog("Desligamento%20do%20sistema");
  } else if(pump_on == 0){
      system_mode = 2; // Manual
      if(old_mode != system_mode)
        sendLog("Bomba%20ligada%20manualmente");
      startPump(false); // start pump but not send log
  } else {
      system_mode = 1; // Automatic
      if(old_mode != system_mode)
        sendLog("Modo%20automatico%20ativado");
       // check for water level
      if (nivel <= confs.minLimit){
        startPump(true); // start pump and send log
      } else if (nivel >= confs.maxLimit){  
        shutdownPump();
      }
  }

  lcd.setCursor(0,1);
  if (nivel >= 100)
   nivel = 100;
  else if (nivel <=0) 
    nivel = 0;
    
  lcd.print(nivel);
  lcd.print("%  ");
  
   // display webpage
  //serveHttpServer();
}

void startPump(boolean send_log){
  if(bomba_estado)
    return;

  bomba_estado = 1;
  digitalWrite(rele, HIGH);
  lcd.setCursor(10,1);
  pump_on(12,1);
  if(send_log)
    sendLog("Bomba%20ligada%20automaticamente");
}

void shutdownPump(){
  if(!bomba_estado)
    return;
   
  bomba_estado = 0;
  digitalWrite(rele, LOW);
  lcd.setCursor(10,1);
  pump_off(12,1);
  sendLog("Bomba%20desligada%20automaticamente");
}

/* funcao que envia o monitoramento para o servidor na internet
void serveHttpServer(){
//  EthernetClient c = server.available();
  if(c){
     c.println("HTTP/1.1 200 OK");
     c.println("Content-Type: text/html");
     c.println("Access-Control-Allow-Origin: *");
     c.println();
     server.print("current_level:");
     server.print(nivel);
     server.print(";");
     server.print("pump_state:");
     server.print((bomba_estado ? 75 : 25));
     server.print(";");
     server.print("system_mode:");
     server.print(system_mode);
     c.stop();
  }
}
*/
boolean loadConfigs(){
  if(SD.begin(5)){
    cfile = SD.open("confs.txt", FILE_READ);
    if(cfile){
      String configname;
      String configval;
      boolean is_value = false;
      while(cfile.available()){
        char n = (char) cfile.read();
        if(!is_value){
          if(n != '='){
            configname += n;
            is_value = false;
          }
          else
            is_value = true;
        }
        else{
          if(n != ';')
            configval += n;
          else{
            setConfig(configname, configval);
            configname = "";
            configval = "";
            is_value = false;
          }
        }
      }
      cfile.close(); 
      return true;
    } 
  }  

  return false;
}

void setConfig(String confname, String confval){
  if(confname == "minlimit")
    confs.minLimit = atoi(confval.c_str());
  else if(confname == "maxlimit")
    confs.maxLimit = atoi(confval.c_str());
  else if(confname == "wifissid")
    confs.wifi.netname = confval; 
  else if(confname == "wifipassword")
    confs.wifi.netpwd = confval;
  else{
    int i, cont;
    int ctotal = confval.length();
    String split = "";
    byte *container;
    if(confname == "netip")
      container = confs.network.ip;
    else if(confname == "netdns")
       container = confs.network.dns_srv;
    else if(confname == "netgateway")
      container = confs.network.gateway;
    else if(confname == "netmask")
       container = confs.network.mask;
   
    for(i;i<=ctotal;i++){
      if(confval.c_str()[i] == '.' || i == ctotal){
        container[cont] = atoi(split.c_str());
        cont++;
        split = "";
      }
      else
        split += confval.c_str()[i];
    }
  } 
}

// funcao que envia o log para o servidor na internet
void sendLog(String logtxt){
   //if(client_log.connect(log_srv, 80)){
    // client_log.print("GET /actions.php?action=RL&log=");
     //client_log.print(logtxt);
    // client_log.println(" HTTP/1.1");
     //client_log.println("Host: agua.silvamedina.com.br");
    // client_log.println("Connection: close");
    // client_log.println();
     //client_log.flush();
    // client_log.stop();
  // }
}

// funcao que exibe a bomba desabilitada
void pump_disable(int col, int row){
  // exibe os simbolos da bomba parada, informe col e row no centro do rotor
  lcd.setCursor(col-1,row);
  lcd.write((byte)0);
  lcd.write("X");
  lcd.write((byte)1);
}

// funcao que exibe a bomba desligada
void pump_off(int col, int row){
  // exibe os simbolos da bomba parada, informe col e row no centro do rotor
  lcd.setCursor(col-1,row);
  lcd.write((byte)0);
  lcd.write("|");
  lcd.write((byte)1);
}

// funcao que exibe a bomba ligada
void pump_on(int col, int row){
  // exibe os simbolos da bomba girando, informe col e row no centro do rotor
  lcd.setCursor(col-1,row);
  lcd.write((byte)0);
  lcd.write((byte)5);
  lcd.write((byte)1);
  lcd.setCursor(col-2,row);
  lcd.write((byte)2);
  delay(x);
  
  lcd.setCursor(col,row);
  lcd.write("|");
  lcd.setCursor(col-2,row);
  lcd.write((byte)3);
  delay(x);

  lcd.setCursor(col,row);
  lcd.write("/");
  lcd.setCursor(col-2,row);
  lcd.write((byte)4);
  delay(x);
  
  lcd.setCursor(col,row);
  lcd.write("-");
  lcd.setCursor(col-2,row);
  lcd.write((byte)4);
  lcd.setCursor(col+2,row);
  lcd.write((byte)2);
  delay(x); 
  
  lcd.setCursor(col,row);
  lcd.write((byte)5);
  lcd.setCursor(col-2,row);
  lcd.write((byte)4);
  lcd.setCursor(col+2,row);
  lcd.write((byte)3);
  delay(x); 
  
  lcd.setCursor(col,row);
  lcd.write("|");
  lcd.setCursor(col-2,row);
  lcd.write((byte)4);
  lcd.setCursor(col+2,row);
  lcd.write((byte)4);
  delay(x); 

  lcd.setCursor(col,row);
  lcd.write("/");
  delay(x); 
  
  lcd.setCursor(col,row);
  lcd.write("-");
  delay(x); 
  
  lcd.setCursor(col-2,row);
  lcd.print(" ");
  lcd.setCursor(col+2,row);
  lcd.print(" ");
}
