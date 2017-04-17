#ifndef CaixaDagua_h
#define CaixaDagua_h

#include "arduino.h"

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
