#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69

#define NODEID        2    // keep UNIQUE for each node on same network
#define NETWORKID     100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID     1    // "central" node

//#define FREQUENCY   RF69_433MHZ
#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ

//#define FREQUENCY_EXACT 916000000 // you may define an exact frequency/channel in Hz
//#define ENCRYPTKEY    "" //exactly the same 16 characters/bytes on all nodes!
//#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!

//#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80

//----------------------------------------------------------------
#define SERIAL_BAUD   115200

//----------------------------------------------------------------
int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)
char buff[50];
boolean requestACK = true;
int reset_pin = 3;

//----------------------------------------------------------------
void ResetRFM()
{
  pinMode(reset_pin, OUTPUT);
  digitalWrite(reset_pin,HIGH);
  delay(100);
  digitalWrite(reset_pin,LOW);
  delay(100);
}

//----------------------------------------------------------------
#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

//----------------------------------------------------------------
void setup() 
{
  ResetRFM();

  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif

#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT); //set frequency to some custom frequency
#endif
  
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  char buff[50];
  sprintf(buff, "\nThis node ID: %d, network ID: %d", NODEID, NETWORKID);
  Serial.println(buff);

  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif
}

//----------------------------------------------------------------
void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

//----------------------------------------------------------------
byte getTemp()
{
      byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
      return temperature;
}

//----------------------------------------------------------------
long lastPeriod = 0;
uint32_t packetCount = 0;

//----------------------------------------------------------------
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();

    if (input == 'r')
      radio.readAllRegs();

    if (input == 't')
    {
      byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
      byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
      Serial.print( "Radio Temp is ");
      Serial.print(temperature);
      Serial.print("C, ");
      Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
      Serial.println('F');
    }
  }

  int currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod = currPeriod;

    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
  
    sprintf(buff, "Temperature is %d", getTemp(), DEC);
    Serial.print("Sending to [");
    Serial.print(GATEWAYID);
    Serial.print("] Data: ");
    Serial.print(buff);
    //Serial.println();
    
    byte buffLen = strlen(buff);

    if (radio.sendWithRetry(GATEWAYID, buff, buffLen, 3))
      Serial.print(" ACK received!");
    else 
      Serial.print(" no ACK received...");

    Serial.println();
    Blink(LED_BUILTIN,1000);
  }
}
