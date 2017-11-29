#include <SPI.h>
#include <PN532.h>
#include <NFCLinkLayer.h>
#include <SNEP.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include <NdefMessage.h>
#define DEBUG true
#define SS 10

PN532 nfc(SS);
NFCLinkLayer linkLayer(&nfc);
SNEP snep(&linkLayer);

uint32_t createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage);

// This message shall be used to rx or tx 
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE  50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr; 
uint8_t *rxNDEFMessagePtr; 
uint8_t txLen;


uint8_t flag=0;
uint8_t *msg;

#define SHORT_RECORD_TYPE_LEN   0x0A
#define NDEF_SHORT_RECORD_MESSAGE_HDR_LEN   0x03 + SHORT_RECORD_TYPE_LEN
#define TYPE_STR "text/plain"

void phoneInRange()
{
  //sleep_disable(); // Prevents the arduino from going to sleep if it was about too. 
}
void(* resetFunc) (void) = 0;
/************************KEYPAD**********************************/
#include <Keypad.h>

const byte numRows= 4; //number of rows on the keypad
const byte numCols= 4; //number of columns on the keypad

//keymap defines the key pressed according to the row and columns just as appears on the keypad
char keymap[numRows][numCols]= 
{
{'1', '2', '3', 'A'}, 
{'4', '5', '6', 'B'}, 
{'7', '8', '9', 'C'},
{'*', '0', '#', 'D'}
};

String Users[4]={"Moh_123","Net_456","Sha_789","Jay_555"};
int bal[4]={50,2000,667,1000};
//Code that shows the the keypad connections to the arduino terminals
byte colPins[numRows] = {A1,2,A2,4}; //Rows 0 to 3
byte rowPins[numCols]= {A4,7,A5,8}; //Columns 0 to 3

//initializes an instance of the Keypad class
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);


String keypad_input="";
int flag_break = 1;

/********************************LCD********************************/
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(A0, 3, A3, 5, 6, 9);


void setup(void) {
    Serial.begin(115200);
    Serial1.begin(115200);
    lcd.begin(16, 2);
    Serial.println("----------------- nfc ndef demo --------------------");

    
       sendData("AT+RST\r\n",2000,DEBUG); // reset module
       sendData("AT+CWMODE=2\r\n",1000,DEBUG); // configure as access point
      sendData("AT+CIFSR\r\n",1000,DEBUG); // get ip address
      sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
     sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
  
    
    
    uint8_t message[] = "1";
    txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
    rxNDEFMessagePtr = &rxNDEFMessage[0];
    txLen = createNDEFShortRecord(message, sizeof(message), txNDEFMessagePtr);    
    
    if (!txLen)
    { 
        Serial.println("Failed to create NDEF Message.");
        while(true); //halt
    }
    
    
    nfc.initializeReader();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    Serial.print("Supports "); Serial.println(versiondata & 0xFF, HEX);
    
#ifdef ENABLE_SLEEP    
    // set power sleep mode
    set_sleep_mode(SLEEP_MODE_ADC);
    // interrupt to wake MCU
    attachInterrupt(0, phoneInRange, FALLING);
#endif
    
    nfc.SAMConfig();
}

uint8_t buf[180];

void loop(void) 
{
   Serial.println();
   Serial.println(F("---------------- LOOP ----------------------"));
   Serial.println();

    uint32_t rxResult = GEN_ERROR; 
    uint32_t txResult = GEN_ERROR;
    rxNDEFMessagePtr = &rxNDEFMessage[0];
    
    
#ifdef ENABLE_SLEEP
     if (IS_ERROR(nfc.configurePeerAsTarget(SNEP_SERVER))) {
        sleepMCU();
        
        extern uint8_t pn532_packetbuffer[];
        nfc.readspicommand(PN532_TGINITASTARGET, (PN532_CMD_RESPONSE *)pn532_packetbuffer);
     }
#else
     if (IS_ERROR(nfc.configurePeerAsTarget(SNEP_SERVER))) {
        extern uint8_t pn532_packetbuffer[];
        
        while (!nfc.isReady()) {
        }
        nfc.readspicommand(PN532_TGINITASTARGET, (PN532_CMD_RESPONSE *)pn532_packetbuffer);
      }
#endif
    
    do {
        //Serial.println("Begin Rx Loop");
        rxResult = snep.rxNDEFPayload(rxNDEFMessagePtr);
        
        if (rxResult == SEND_COMMAND_RX_TIMEOUT_ERROR)
        {
          Serial.println("rxNDEFPayload() timeout");
           break;
        } else if (IS_ERROR(rxResult)) {
          Serial.println("rxNDEFPlayload() failed");
          break;
        }  
   
    
        if (RESULT_OK(rxResult) && flag==0)
        {
           NdefMessage *message = new NdefMessage(rxNDEFMessagePtr, rxResult);
           Serial.print("NDEF record: ");
           Serial.println(message->getRecordCount());
           NdefRecord record = message->getRecord(0);
           Serial.println(" ");
           msg=record.getPayload();
           //Serial.println((char*)msg);
           if((char)(msg[1]=='e') && ((char)msg[2]=='n') )
           {
              //int tflag=0;
              int cmpr=0;
              int q=0;
              int l=0;
              for(q=0;q<4;q++)
              {
                for(l=0;l<7;l++)
                {
                  if(msg[l+3]!=Users[q].charAt(l))
                  {
                    break; 
                  }
                }
                if(l==7)
                {
                  cmpr=q;
                  break;
                }
              }
              if(q==4)
              {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Access Denied");
                delay(5000);
                resetFunc();
              }
              Serial.println((char*)msg);
              Serial.print("User=");
              Serial.println(q);
              //txResult = snep.pushPayload(txNDEFMessagePtr, txLen);
              flag=1; 
              //from internet database verify pin, yet to code
              lcd.setCursor(0, 0);
              lcd.print("Verified");
              //LCD display remaining - verified:
              delay(10000);
              lcd.setCursor(0, 0);
              lcd.print("Amount entered:");
               /******************Keypad input*************/
               String bal_in="";
               while(flag_break){
               char keypressed = myKeypad.getKey();
               lcd.setCursor(0,1);
                if (keypressed != NO_KEY)
                {
                Serial.print(keypressed);
                
                if(keypressed == '*')
                {
                  lcd.setCursor(0, 1);
                  if(bal[q]<bal_in.toInt())
                  {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Bad credit");
                    lcd.setCursor(0, 1);
                    lcd.print("Resetting");
                    delay(5000);
                    resetFunc();
                  }
                  lcd.print("Payment processed");
                  
                  while(true)
                  {
                      if(Serial1.available()) // check if the esp is sending a message 
                      {
                        
                        if(Serial1.find("+IPD,"))
                        {
                         delay(1000);
                     
                         int connectionId = Serial1.read()-48; // subtract 48 because the read() function returns 
                                                             // the ASCII decimal value and 0 (the first decimal number) starts at 48
                       
                       String webpage = "Successful Transaction of ";
                       webpage+=bal_in;
                       webpage+=" rupees";
                       webpage+=" Curr bal=";
                       webpage+=String(bal[q]-bal_in.toInt());
                       //webpage+="</h2>";                    
                       String cipSend = "AT+CIPSEND=";
                       cipSend += connectionId;
                       cipSend += ",";
                       cipSend +=webpage.length();
                       cipSend +="\r\n";
                       
                       sendData(cipSend,1000,DEBUG);
                       sendData(webpage,1000,DEBUG);
                       
                       /*webpage="<button>LED2</button>";
                       
                       cipSend = "AT+CIPSEND=";
                       cipSend += connectionId;
                       cipSend += ",";
                       cipSend +=webpage.length();
                       cipSend +="\r\n";
                       
                       sendData(cipSend,1000,DEBUG);
                       sendData(webpage,1000,DEBUG);*/
                   
                       String closeCommand = "AT+CIPCLOSE="; 
                       closeCommand+=connectionId; // append connection id
                       closeCommand+="\r\n";
                       
                       sendData(closeCommand,3000,DEBUG);
                      }
                    }
                  }

                  
                  flag_break = 0;
                  break;
                }
                bal_in+=keypressed;
                keypad_input+=keypressed;
                }
                lcd.print(keypad_input);
               }           
           }

           /* yet to check validity of amount entered - only numbers and then check balance and print successful*/
           
           else
           {
              /*if((char)(msg[1]=='e') && ((char)msg[2]=='n'))
              {
                
              }*/
           }
           //record.print();
        }
     
        
      
        delay(3000);   
     } while(0);
     
     
}

uint32_t createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage)
{
   //Serial.print("Message: ");
   //Serial.println((char *)message);
   uint8_t * NDEFMessageHdr = ALLOCATE_HEADER_SPACE(NDEFMessage, NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);
   
   NDEFMessageHdr[0] =  NDEF_MESSAGE_BEGIN_FLAG | NDEF_MESSAGE_END_FLAG | NDEF_MESSAGE_SHORT_RECORD | TYPE_FORMAT_MEDIA_TYPE; 
   NDEFMessageHdr[1] =  SHORT_RECORD_TYPE_LEN;
   NDEFMessageHdr[2] =  payloadLen;
   memcpy(&NDEFMessageHdr[3], TYPE_STR, SHORT_RECORD_TYPE_LEN);
   memcpy(NDEFMessage, message, payloadLen);
   //Serial.print("NDEF Message: ");
   //Serial.println((char *)NDEFMessage);   
   NDEFMessage = NDEFMessageHdr;
   return (payloadLen + NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);   
}
int mycmp(uint8_t *s1,uint8_t *s2)
{
  int flag1=0;
  for(int i=3;i<6;i++)
  {  
    if(s1[i]!=s2[i-3])
    {
      flag1=1;
      break;
    }
  }
  return flag1;   
}
void sleepMCU()
{
    delay(100);  // delay so that debug message can be printed before the MCU goes to sleep
    
    // Enable sleep mode
    sleep_enable();          
    
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer1_disable();
    power_timer2_disable();
    power_twi_disable();
    
    //Serial.println("Going to Sleep\n");
    //delay(1000);
    
    // Puts the device to sleep.
    sleep_mode();  
    Serial.println("Woke up");          
    
    // Program continues execution HERE
    // when an interrupt is recieved.

    // Disable sleep mode
    sleep_disable();         
    
    power_all_enable();
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    
    Serial1.print(command); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(Serial1.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = Serial1.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
      Serial.print(response);
    }
    
    return response;
}
   
