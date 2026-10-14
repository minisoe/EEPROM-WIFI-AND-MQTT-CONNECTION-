#include <Preferences.h>
Preferences preferences;

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n-- EEPROM Read and Write --");
  //storage is namespae for Preferences, false is Read/Write mode
  preferences.begin("storage",false);
  //if previous saved data, read the counter
  //if it's doesn't declare the clonter to 0
  unsigned int counter = preferences.getUInt("counter",0);
  counter++;
  preferences.putUInt("counter",counter);
  Serial.print("Restart Count (Value read from EEPROM):");
  Serial.println(counter);

}
void loop(){

}