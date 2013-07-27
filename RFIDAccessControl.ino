/*
Author: Santiago Castro Castrillon

*/

// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif


#include <EEPROM.h>
#include <EEPROMList.h>
#include <SoftwareSerial.h>

#define USE_RFIDEVAL // comment this line to disable rfid eval shield
//#define USE_RFID125 // comment this line to disable rfid 125KHz reader
// Now it's not posible to use both readers simultaneously, I'm testing this posibility


#define GREEN_LED 13
#define RED_LED 12

#define BUTTON 2

#define ITEM_SIZE 6
EEPROMList list(ITEM_SIZE);


/*
 Definitions for high frecuency RFID Eval 13,56MHz Shield (https://www.sparkfun.com/products/10406)
 */
#define SOFT_SERIAL_RFIDEVAL_RX 7
#define SOFT_SERIAL_RFIDEVAL_TX 8

#define HALT_COMMAND (uint8_t)0x93
#define SEEK_COMMAND (uint8_t)0x82

#if defined(USE_RFIDEVAL)
SoftwareSerial rfid_eval(SOFT_SERIAL_RFIDEVAL_RX, SOFT_SERIAL_RFIDEVAL_TX);
#endif

#if defined(USE_RFID125)
/*
 Definitions for low frecuency 125KHz reader (https://www.sparkfun.com/products/9963)
 */
#define SOFT_SERIAL_RFID125_RX 10
#define SOFT_SERIAL_RFID125_TX 11

SoftwareSerial rfid_125(SOFT_SERIAL_RFID125_RX, SOFT_SERIAL_RFID125_TX);
#endif


#define RFID_START_CHAR  (char)10
#define RFID_END_CHAR  (char)11


void setup() {
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  Serial.begin(9600);

#if defined(USE_RFIDEVAL)
  Serial.println("RFID Eval shield initialized");
  setup_rfideval();
#endif

#if defined(USE_RFID125)
  Serial.println("RFID 125KHz reader initialized");
  setup_rfid125();
#endif

  Serial.print("Current user list size: ");
  Serial.println(list.size());

}

void loop() {
#if defined(USE_RFIDEVAL)
  rfid_eval.listen();
  delay(5);
  if(check_rfideval()){
    processRFIDCode();
  }
  delay(200);
#endif

#if defined(USE_RFID125)
  rfid_125.listen();
  delay(5);
  if(check_rfid125()){
    processRFIDCode();
    delay(200);
  }
#endif
}

uint8_t hex_card_id[ITEM_SIZE];
void processRFIDCode(){
  // print card id
  Serial.print("Card id beeing proccessed: ");
  for(uint8_t i=0; i<ITEM_SIZE; i++){
    Serial.print((uint8_t)hex_card_id[i], HEX);
    Serial.print(" ");
  }  
  Serial.println();

  if(digitalRead(BUTTON)==LOW){ //if the button is pressed, then the code is added to eeprom
    list.add((byte*) hex_card_id);
  }
  else{
    if(list.exists((byte*) hex_card_id)){
      accessAccepted();
    }
    else{
      accessDenied();
    }
  }
}

// Actions taken when an accepted card is read
void accessAccepted(){
  digitalWrite(GREEN_LED, HIGH);
  Serial.print("Accepted: ");
  Serial.println("YES");
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
}

// Actions taken when a not recognized card is read
void accessDenied(){
  digitalWrite(RED_LED, HIGH);
  Serial.print("Accepted: ");
  Serial.println("NO");
  delay(1000);
  digitalWrite(RED_LED, LOW);
}




/* ------------------------------------------------------
 ---- FUNTIONS FOR 125KHZ READER ----------------------
 ------------------------------------------------------
 */
#if defined(USE_RFID125)
void setup_rfid125(){
  rfid_125.begin(9600);
}

#define MAX_125_BUFFER_LENGTH 16
char buffer_125[MAX_125_BUFFER_LENGTH];

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

void convert_to_hex(){
  for(uint8_t i=0; i<6; i++){
    //the ascii characters are converted to hex bytes to save half the space
    char c1 = buffer_125[i*2];
    char c2 = buffer_125[i*2+1];
    hex_card_id[i] = (char2hex(c1)<<4) | (char2hex(c2) & 15);
  }
}

/*
The rfid_eval reader sends ascii 2 (start of text), 12 ascii characters of hexadecimal values, 
 carriage return, new line and ascii 3 (end of text).
 */
int bufferLength;
boolean check_rfid125(){
  char incomingByte;
  if (rfid_125.available()) {
    // wait a bit for the entire message to arrive
    delay(50);
    // read all the available characters
    while (rfid_125.available() > 0) {
      incomingByte = rfid_125.read();

      if(incomingByte == RFID_END_CHAR){ 
        //If MAX_CODE_LENGTH is reached, then ignore
        if(bufferLength < MAX_125_BUFFER_LENGTH){
          process_RFID125_code();
          bufferLength = MAX_125_BUFFER_LENGTH;
        }
      }
      else{
        if(incomingByte == RFID_START_CHAR)
        {
          bufferLength=0;
        }
        else if(bufferLength < MAX_125_BUFFER_LENGTH){
          buffer_125[bufferLength] = incomingByte;
          bufferLength++;
        }
      }
    } 
  }
}

boolean process_RFID125_code(){
  if(validate_125_code()){
    convert_to_hex();
    processRFIDCode();
  }
  else{
    Serial.println("Invalid");
  }
}

boolean validate_125_code(){
  // I ignore char 13 and 10 on positions 7 and 8 sent by the reader. I suppose that
  // other readers don't send these characters (cr lf)
  return(bufferLength==14 || buffer_125[12]==(char)13 || buffer_125[13]==(char)10);
}

void simulateRead(){
  Serial.println("simulatedRead");
  strcpy(buffer_125, "0123456789ab");
  buffer_125[12] = 13;
  buffer_125[13] = 10;
  bufferLength=14;
  process_RFID125_code();
}
#endif



#if defined(USE_RFIDEVAL)
/* ------------------------------------------------------
 ---- FUNTIONS FOR 13.56MHZ SHIELD --------------------
 ------------------------------------------------------
 */
#define MAX_EVAL_BUFFER_LENGTH 11
uint8_t buffer_eval[MAX_EVAL_BUFFER_LENGTH];

void setup_rfideval(){
  rfid_eval.begin(19200);
  delay(10);
  send_command(HALT_COMMAND);
}
boolean check_rfideval()
{
  //if( rfid_eval.available() ){
  //  Serial.println("there is data on buffer before check");
  // rfid_eval.flush();
  //}
  send_command(SEEK_COMMAND);
  delay(10);
  return parse_rfideval();
}

boolean parse_rfideval()
{
  buffer_eval[2] = 0;
  while(rfid_eval.available()){
    if(rfid_eval.read() == 255){
      for(int i=1;i<11;i++){
        buffer_eval[i]= rfid_eval.read();
      }
    }
  }
  if(validate_eval_code()){
    //    Serial.println("card id received");
    //this means that a card id was received
    hex_card_id[0]= buffer_eval[8];
    hex_card_id[1]= buffer_eval[7];
    hex_card_id[2]= buffer_eval[6];
    hex_card_id[3]= buffer_eval[5];
    for(int i=4; i<ITEM_SIZE; i++){
      hex_card_id[i]=0;
    }
    return true;
  }
  else if(buffer_eval[2] == 2){
    //    Serial.println("reader is waiting");
  }
  else if(buffer_eval[2] == 0){
    //    Serial.println("response not received");
  }
  else{
//    Serial.print("invalid response");
  }
  return false;
}

boolean validate_eval_code(){
  if(buffer_eval[2] == 6){
    //checksum validation
    uint8_t checksum=0;
    for(int i=1; i<9; i++){
      checksum += buffer_eval[i];
    }
    return checksum == buffer_eval[9];
  }
  return false;
}

void send_command(uint8_t command_code)
{
  rfid_eval.write((uint8_t)255);
  rfid_eval.write((uint8_t)0);
  rfid_eval.write((uint8_t)1);
  rfid_eval.write(command_code);
  rfid_eval.write((uint8_t)(command_code+1));
  delay(10);
}
#endif
