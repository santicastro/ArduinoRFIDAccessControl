RFIDAccessControl
=================

Simple RFID Access Control that stores valid users on eeprom.

The RFID Card Reader used sends the RFID code over serial port with this format: 
   SHHHHHHHHHHHHCLE
with:
S: ascii value 2, Start of text
H: 12 hexadecimal ascii characters (so byte 0xAB is sent as 2 different chars, 'A' and 'B')
C: ascii value 13, Carriage Return
L: ascii value 10, New Line
E: ascii value 3, End of text 


It depends on library EEPROMList found here: https://github.com/santicastro/EEPROMList
