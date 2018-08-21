#include "menu.h"

void printAllValveState(
                        LiquidCrystal * lcd , 
                        byte * valvesState, 
                        byte * valvesScheduled 
                       )
{ 
  int i=0;
  lcd->print("");
  for (i=0; i<8; i++) { lcd->print((int) i); lcd->print(" "); } 
  lcd->setCursor(0, 1);
  lcd->print("");  
  for (i=0; i<8; i++) 
               { 
                  if (valvesState[i])     { lcd->print((char)ON_LOGO);  } else  
                  if (valvesScheduled[i]) { lcd->print((char) 'W');  } else  
                                          { lcd->print((char)OFF_LOGO); } 
                  lcd->print(" "); 
               }  
}
