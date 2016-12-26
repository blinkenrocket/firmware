
unsigned long t,t_old;

void setup() {
  // put your setup code here, to run once:

  DDRB =(1<<4);  // MISO output
  SPCR = (1<<SPE);   // SPI slave mode

  Serial.begin(9600);
  Serial.println("Blinkenrocket SPI test program");
    
}

uint8_t SHOWBYTES=1;

void loop() {
  uint8_t c;
  static uint8_t l=0;
  // put your main code here, to run repeatedly:
  SPDR = 'X';
  t=millis();
  while(!(SPSR & (1<<SPIF) ));
  c=SPDR;
  if (millis()-t > 300) 
  {
    Serial.println(" ");
    Serial.println("-----------------");
    l=0;  
  }
  if (SHOWBYTES) {
      l++; 
      if (l%3) {   // do not display hamming code checksums
        if (((c>='a') && (c<='z')) || ((c>='A') && (c<='Z')) || ((c>='0') && (c<='9')) || (c=='!') || (c==' '))
        {   Serial.print(" "); Serial.print((char)c); }
        else { if (c <16) Serial.print("0"); Serial.print(c,HEX); }
        Serial.print(" ");
      } else l=0;
  } 
  else {
        // if (c <10) Serial.print("0");
        Serial.print(((uint16_t)c));
        Serial.print(" ");
  }
 }
