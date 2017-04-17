#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
/* -----------------------------------------------------------------------------------------
| Struct ConfigManager 
| ConfigManager: (int pumpStartLimit, int pumpStopLimit) 
 ------------------------------------------------------------------------------------------- */
struct ConfigManager {
  int pumpStartLimit = 10;
  int pumpStopLimit = 100;
};
/* ------------------------------------------------------------------------ 
| Constantes em tempo de compilação 
 -------------------------------------------------------------------------- */
#define trig 2
#define echo 3
#define rele 4
#define modo1 6
#define modo2 7
/* ------------------------------------------------------------------------ 
| Global variables 
 -------------------------------------------------------------------------- */
File cfile;
LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3, POSITIVE);
// define o tempo em milesegundos entre os frames
int animation_frame = 200;
// define os simbolos da bomda e agua
byte bomba_e[8] = {B00001,B00010,B11100,B11100,B11100,B11100,B00010,B00001};
byte bomba_d[8] = {B10000,B01000,B00111,B00111,B00111,B00111,B01000,B10000};
byte agua[8] = {B00000,B00000,B10101,B00000,B00000,B10101,B00000,B00000};
int nivel, bomba_estado = 0;
int disable = 0;
int system_mode = 0;
int old_mode = -1;
signed int nivel_media[3] = {-500, -500, -500};
int media_count = 0;
ConfigManager confs;
int nivel_control = -200;
int clock_medicao = 0;

/* ------------------------------------------------------------------------ 
| setup
| @do: initializes arduino logic
 -------------------------------------------------------------------------- */
void setup() {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(rele, OUTPUT);

  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Inicializando");
  lcd.setCursor(4,1);
  lcd.print("Sistema");

  delay(2000);
  lcd.clear();
  if(!loadConfigs()){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Configs error");
    delay(5000);
  }
   
  delay(2000);
// crias so simbolos da bomda e agua
  lcd.createChar(0, bomba_e);  
  lcd.createChar(1, bomba_d);  
  lcd.createChar(2, agua);

// limpa o display de LCD
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Nivel  |  Bomba"); 
}
/* ------------------------------------------------------------------------ 
| loop
| @do: controls components and logic each time it's called
 -------------------------------------------------------------------------- */
void loop() {
  digitalWrite(trig, HIGH);
  delay(10);
  digitalWrite(trig, LOW);
  int tempo = pulseIn(echo, HIGH);
  int distancia = tempo/58;

//  lcd.setCursor(0,1);  
//  lcd.print(distancia);
//  lcd.print("cm");
//  return;
  
  nivel = map(distancia, 51, 4, 0, 100);

  clock_medicao++;
  
  if(nivel_control < -100)
    nivel_control = nivel;
  //else if(nivel%nivel_control >= 5)
  else if(clock_medicao >= 300 && ((bomba_estado && nivel > nivel_control) || (!bomba_estado && nivel < nivel_control)) && abs(nivel-nivel_control) >= 2){
    nivel_control = nivel;
    clock_medicao = 0;
  }
  
//  if (nivel >= 100)
//   nivel = 100;
//  else if (nivel <=0) 
//   nivel = 0;
     
  lcd.setCursor(0,1);  
  lcd.print(nivel_control);
  lcd.print("%  ");
  
  if (digitalRead(modo1) == HIGH && digitalRead(modo2) == LOW ){
      system_mode = 1;
      lcd.setCursor(7,1);
      lcd.print("A");
      if (nivel_control <= confs.pumpStartLimit){
         digitalWrite(rele, HIGH);
         bomba_estado = 1;
      } else if (nivel_control >= confs.pumpStopLimit){
        bomba_estado = 0;
        digitalWrite(rele, LOW);
      }
    } else if (digitalRead(modo1) == LOW && digitalRead(modo2) == HIGH ){
      system_mode = 2;
      lcd.setCursor(7,1);
      lcd.print("L");
      bomba_estado = 1;
      digitalWrite(rele, HIGH);
    } else {
      system_mode = 0;
      lcd.setCursor(7,1);
      lcd.print("D");
      bomba_estado = 0;
      digitalWrite(rele, LOW);
   }

    lcd.setCursor(10, 1);
    if (system_mode == 0)
      pump_disable(12,1);
    else if ((system_mode == 1 && bomba_estado == 1) || system_mode == 2)
      pump_on(12,1);
    else if (system_mode == 1  && bomba_estado == 0) 
     pump_off(12,1);
}

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
    confs.pumpStartLimit = atoi(confval.c_str());
  else if(confname == "maxlimit")
    confs.pumpStopLimit = atoi(confval.c_str()); 
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
  lcd.setCursor(col-2,row);
  lcd.write((byte)2);
  lcd.write((byte)0);
  lcd.write((byte)2);
  lcd.write((byte)1);
  lcd.write((byte)2);
  delay(animation_frame);
  
  lcd.setCursor(col-2,row);
  lcd.print(" ");
  lcd.setCursor(col,row);
  lcd.print(" ");
  lcd.setCursor(col+2,row);
  lcd.print(" ");
}
