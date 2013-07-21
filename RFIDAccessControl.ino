#include <EEPROM.h>
#include <EEPROMList.h>
#include <SoftwareSerial.h>

#define ITEM_SIZE 6

#define GREEN_LED 13
#define RED_LED 12

#define BUTTON 2

#define RFID_START_CHAR  (char)10
#define RFID_END_CHAR  (char)11


SoftwareSerial rfidSerial(10, 11);
EEPROMList list(ITEM_SIZE);

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  rfidSerial.begin(9600);

  //test code
  uint8_t it[6] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab          };
  list.add(it);

  Serial.print("List size: ");
  Serial.println(list.size());
}
const int MAX_CODE_LENGTH = 16;
char buffer[MAX_CODE_LENGTH], bufferLength;  

void loop() {
  readRFIDCode();
  simulateRead();
}

void simulateRead(){
  strcpy(buffer,"0123456789ab");
  buffer[12]=13;
  buffer[13]=10;
  buffer[14]=0;
  bufferLength=14;
  processRFIDCode(buffer, bufferLength);

  strcpy(buffer,"abcdef012345");
  buffer[12]=13;
  buffer[13]=10;
  buffer[14]=0;

  processRFIDCode(buffer, bufferLength);
  delay(2000);
}


/*
The rfid reader sends ascii 2 (start of text), 12 ascii characters of hexadecimal values, 
carriage return, new line and ascii 3 (end of text).
*/
void readRFIDCode(){
  char incomingByte;
  if (Serial.available()) {
    // wait a bit for the entire message to arrive
    delay(50);
    // read all the available characters
    while (Serial.available() > 0) {
      incomingByte = Serial.read();
      if(incomingByte == RFID_END_CHAR){ 
        //If MAX_CODE_LENGTH is reached, then ignore
        if(bufferLength < MAX_CODE_LENGTH){
          buffer[bufferLength]=0;
          processRFIDCode(buffer, bufferLength);
          bufferLength = MAX_CODE_LENGTH;
        }
      }
      else{
        if(incomingByte == RFID_START_CHAR)
        {
          bufferLength=0;
        }
        else if(bufferLength < MAX_CODE_LENGTH){
          buffer[bufferLength] = incomingByte;
          bufferLength++;
        }
      }
    } 
  }
}

uint8_t rfidCode[ITEM_SIZE];
void processRFIDCode(char* buffer, int bufferLength){
  // I ignore char 13 and 10 on positions 7 and 8 sent by the reader. I suppose that
  // other readers don't send these characters (cr lf)
  if(bufferLength!=14 || buffer[12]!=(char)13 || buffer[13]!=(char)10){
    Serial.println("A RFID code was received but the format was not correct");
  }
  else{
    buffer[12]=0;
    for(uint8_t i=0; i<ITEM_SIZE; i++){
      //the ascii characters are converted to hex bytes to save half the space
      char c1 = buffer[i*2];
      char c2 = buffer[i*2+1];
      rfidCode[i] = (char2hex(c1)<<4) | (char2hex(c2) & 15);
    }
    if(digitalRead(BUTTON)==LOW){ //if the button is pressed, then the code is added to eeprom
      list.add(rfidCode);
    }
    else{
      if(list.exists(rfidCode)){
        accessAccepted(buffer);
      }
      else{
        accessDenied(buffer);
      }
    }
  }
}
void accessAccepted(char* buffer){
  digitalWrite(GREEN_LED, HIGH);
  Serial.print(buffer);
  Serial.print(": ");
  Serial.println("YES");
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
}

void accessDenied(char* buffer){
  digitalWrite(RED_LED, HIGH);
  Serial.print(buffer);
  Serial.print(": ");
  Serial.println("NO");
  delay(1000);
  digitalWrite(RED_LED, LOW);
}

uint8_t char2hex(char c){
  if(c>='a'){//lowercase
    c -= 32;
  }
  if(c<'0' || c>'F'){
    return 0;
  }
  if(c<='9'){
    return (c - '0');
  }
  else if(c>='A'){
    return 10+(c -'A'); 
  }
  return 0;
}






