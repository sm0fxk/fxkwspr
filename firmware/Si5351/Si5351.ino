// WSPR beacon for Arduino Uno and Arduino Nano.
// Author: Ulf Nordstrom, SM0FXK
// This code is inspired by the simple WSPR beacon for Si5351 by Jason Milldrum NT7S
// The code is however heavily modified. The beacon is divided into two parts. One GUI part that runs on a PC,
// and this part, which runs on an Arduino microcontroller. 
// The PC and the Arduino communicates via the serial interface using a CAT command like protocol.
// The code was origilally written for the Si5351 clock generator. Now this code also support the AD8950/AD9851 DDS.
// Which one to use is selected by a constant for conditional compilation.
// The beacon can make use of a hardware clock, RTC. If a RTC is connected, it is enabled by defining 
// the symbol "HW_CLOCK". 
//
// Hardware Requirements
// ---------------------
// This firmware must be run on an Arduino AVR microcontroller
//
// Required Libraries
// ------------------
// Etherkit Si5351 (Library Manager)
// Etherkit JTEncode (Library Manager)
// Time (Library Manager)
// Wire (Arduino Standard Library)
// DS3232RTC (Arduino Standard Library) if clock is enabled
//
/* License
   -------
 * This sketch  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
*/

/*
TODO

- Add 2 more channels


*/

//======================================================================
// Include configuration
//======================================================================
#include "config.h"

//======================================================================
// Include libraries
//======================================================================
#include <EEPROM.h>
#include <TimeLib.h>
#include <JTEncode.h>

#ifdef SI5351
#include <si5351.h>
#include "Wire.h"
#endif

#ifdef AD9850DDS
#include "sa6vee_board.h"
#endif

#ifdef HW_CLOCK
#include <DS3232RTC.h>
#endif

//#include <stddef.h>
//#include <string.h>

#define TONE_SPACING            146           // ~1.46 Hz
#define SYMBOL_COUNT            WSPR_SYMBOL_COUNT
#define SYMBOL_TIME             683            // 683 (1/1.46 Hz)
#define CORRECTION              0             // Change this for your ref osc

#define TIME_REQUEST            7             // ASCII bell character requests a time sync message 

#define PTT_PIN                 7
#define FILTER_80				8
#define FILTER_40				9
#define FILTER_20_17			10
#define FILTER_15_10			11

#define BUFFER_SIZE 80

enum request_type {get_request, set_request};
enum wspr_state_t {inactive, waiting_for_timeslot, on_air, tuning};
#define member_size(type, member) sizeof(((type *)0)->member)

#define LENGTH_MAGIC 4
#define LENGTH_CALL 8
#define LENGTH_LOCATOR 5

typedef struct
{
    char          magic[LENGTH_MAGIC];
    int           xtal_offset;
    char          call[LENGTH_CALL];
    char          locator[LENGTH_LOCATOR];
    int           power;
    unsigned int  band;
    unsigned int  interval;
    wspr_state_t  state;
    char          error_log[40];
} flash_layout;

#define MAGIC "fxk"



typedef struct cmd_entry
{
    char cmd[3];
    void (*fun)(char*);
} cmd_entry;

enum t_status_t {disabled, enabled};

unsigned long timeout_at;
enum t_status_t timer_status = disabled;
void (*callback) ();

// Global variables

unsigned int pos = 0;
unsigned int timeslot = 4;

static void  time_sync(char*);
static void  wspr(char*);
static void  display_log(char*);  // Debug
static void  error(char*);
static void tune(char*);
static void configure_wspr(char*);
static void query_hw(char*);

void call_after (unsigned int, void (*) ());

cmd_entry cmd_table[] = {

                         {"WS", wspr},          // SM0FXK extension. Send WSPR
//                         {"LG", display_log},     // Debug
                         {"TX", tune},       // 
                         {"QC", configure_wspr},
                         {"QT", time_sync},
                         {"QH", query_hw},
                         {"",   error} };
#ifdef SI5351
Si5351 si5351;
#endif
#ifdef AD9850DDS
#define pulse(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW);}

// AD9850 AD9851 selection 
byte w0 = 0;                     // 0 = AD9850, 1 = AD9851 
unsigned long fCLK=125.0e6;    // Enter clock frequency. 125MHz for AD9850. 180MHZ for AD9851

byte data = 6; //DATA 
byte clock = 5; //CLOCK 
byte load = 4; //LOAD 
byte reset;

unsigned long mask = 1; // bitmask
byte temp = 1;
unsigned long FreqWord;
unsigned int OffsetFreq[4];

#endif

JTEncode jtencode;


uint8_t tx_buffer[SYMBOL_COUNT];
uint8_t wspr_symbol;
unsigned int symbol_timer;


//unsigned long ddsclock = 125000000L;
unsigned long freq = 10140100UL;
unsigned int bands;
wspr_state_t wspr_state = inactive;

#ifdef SI5351
si5351_drive output_level = SI5351_DRIVE_4MA; //SI5351_DRIVE_2MA, SI5351_DRIVE_4MA, SI5351_DRIVE_6MA, SI5351_DRIVE_8MA
#endif

// For debug
#define LOG_SIZE 32
//char errorlog[LOG_SIZE];
//int log_write_pos = 0;

char commandbuffer[BUFFER_SIZE];


#ifdef AD9850DDS

void ad9850_init(byte clk_pin, byte fq_ud_pin, byte sdata_pin, byte res_pin)
{
    data  =sdata_pin;
    clock =clk_pin;
    load  =fq_ud_pin;
    reset = res_pin;
    

  
    // Set up interface
  pinMode (data, OUTPUT);   // 
  pinMode (clock, OUTPUT);  // s
  pinMode (load, OUTPUT);   // 
  pinMode (reset, OUTPUT);

  pulse(reset);
  pulse(clock);
  pulse(load);
}

void ad9850_set_frequency(unsigned long TempWord)
{

  for (mask = 1; mask>0; mask <<= 1) { //iterate through bit mask
    if (TempWord & mask){ // if bitwise AND resolves to true
      digitalWrite(data,HIGH); // send 1
    }
    else{ //if bitwise and resolves to false
      digitalWrite(data,LOW); // send 0
    }
    pulse(clock);
  }
  for (temp = 1; temp>0; temp <<= 1) { //iterate through bit mask
    if (w0 & temp){ // if bitwise AND resolves to true
      digitalWrite(data,HIGH); // send 1
    }
    else{ //if bitwise and resolves to false
      digitalWrite(data,LOW); // send 0
    } 
    pulse(clock);

  } 
  pulse(load);
  return;


}

void ad9850_up()
{
//  TempWord = 0;
  ad9850_set_frequency(0);
}

void ad9850_down()
{
  pulse(load);
  uint8_t p = 0x04;

  for (int i = 0; i < 8; i++, p >>= 1)
    {
      digitalWrite(data, p & (uint8_t)0x01);
      pulse(clock);
    }
  pulse(load);
}

#endif

void call_after(unsigned int time, void (*function) ())
{
    timeout_at = millis() + time;
    callback = function;
    timer_status = enabled;
}

void poll_mstimer()
{
  if (timer_status == enabled)
  {
    if (millis() >= timeout_at)
    {
      timer_status = disabled;
      (callback) ();
    }
  }
}


void eestrput(int eeprom_address, char *src)
{
  char c;
  int i=0;
  
  while(c=src[i++])
  {
    EEPROM.write(eeprom_address++, c);

  }
  EEPROM.write(eeprom_address++, 0);

}



void eestrget(int eeprom_address, char *dest, int maxlen)
{
  char c;
  int i = 0;
  
  while(c= EEPROM.read(eeprom_address++))
  {
    if(i > maxlen)
        break;
    dest[i++] = c;

  }
  dest[i++] = 0;

}



void configure_wspr(char* cmd)
{
  char *call;
  char *loc;
  char *power;
  char *offset;
  char *saveptr;

  char tmp_str[8];
  int tmp_int;
  int eeprom_address;

  switch(strlen(cmd))
  {
    case 0:
//       Serial.println("Configure GET");
       eeprom_address = (int)offsetof(flash_layout, magic);
       eestrget(eeprom_address, tmp_str, 4);
       if(strcmp(tmp_str, MAGIC)==0)
       {
           eeprom_address = (int)offsetof(flash_layout, call);
           eestrget(eeprom_address, tmp_str, 7);
           Serial.print(tmp_str); Serial.print(",");

           eeprom_address = (int)offsetof(flash_layout, locator);
           eestrget(eeprom_address, tmp_str, 5);
           Serial.print(tmp_str); Serial.print(",");
           
           eeprom_address = (int)offsetof(flash_layout, power);
           EEPROM.get(eeprom_address, tmp_int);
           Serial.print(tmp_int, DEC); Serial.print(",");
           
           eeprom_address = (int)offsetof(flash_layout, xtal_offset);
           EEPROM.get(eeprom_address, tmp_int);
           Serial.println(tmp_int, DEC);
       }
       else
           Serial.println("NC");
    break;

    
    default:
//       Serial.println("Configure SET");

       
       eeprom_address = (int)offsetof(flash_layout, call);
       call = strtok_r(cmd, ",", &saveptr);
       eestrput(eeprom_address, call);
//       Serial.print("Callsign = "); Serial.println(call);

       eeprom_address = (int)offsetof(flash_layout, locator);
       loc = strtok_r(NULL,  ",", &saveptr); 
       eestrput(eeprom_address, loc);
//       Serial.print("Locator = "); Serial.println(loc);

       eeprom_address = (int)offsetof(flash_layout, power);
       power = strtok_r(NULL,",", &saveptr);
//       Serial.print("Power = "); Serial.println(power);
       EEPROM.put(eeprom_address, atoi(power));
       
       eeprom_address = (int)offsetof(flash_layout, xtal_offset);
       offset = strtok_r(NULL,",", &saveptr);
       EEPROM.put(eeprom_address, atoi(offset));
//     ---------------------------------------------------------------
//     Initiate status, interval, and band

       eeprom_address = (int)offsetof(flash_layout, state);
       EEPROM.put(eeprom_address, inactive);
       eeprom_address = (int)offsetof(flash_layout, interval);
       EEPROM.put(eeprom_address, 2);
       eeprom_address = (int)offsetof(flash_layout, band);
       EEPROM.put(eeprom_address, 80);
       eeprom_address = (int)offsetof(flash_layout, magic);
       eestrput(eeprom_address, MAGIC);
       Serial.println("OK");

       
  break;
  }

}

int processSyncMessage(unsigned long pctime)
{
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  
     if( pctime >= DEFAULT_TIME)
     { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
       return(0);
     }
     return(1);
  
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
 

request_type request(char *param)
{
  if(strlen(param) == 0)
     return(get_request);
  else
     return(set_request);
}


void set_filter(int band)
{
  digitalWrite(FILTER_80, LOW);
  digitalWrite(FILTER_40, LOW);
  digitalWrite(FILTER_20_17, LOW);
  digitalWrite(FILTER_15_10, LOW);
  switch(band)
  {
    case 80:
      digitalWrite(FILTER_80, HIGH);
    break;

    case 40:
      digitalWrite(FILTER_40, HIGH);
    break;

    case 30:
      digitalWrite(FILTER_20_17, HIGH);
    break;

    case 20:
    case 17:
      digitalWrite(FILTER_20_17, HIGH);
    break;

    case 15:
    case 12:
    case 10:
      digitalWrite(FILTER_15_10, HIGH);
    break;
    
    default:
    break;
    
  }
}


//======================================================================
// 
//======================================================================
void time_sync(char *param)
{
  unsigned long pctime;
  char result;
  switch (request(param))
    {
      case get_request:
#ifdef HW_CLOCK
//         Serial.println(RTC.get(), DEC);
         Serial.println(now(), DEC);

#else
         Serial.println(now(), DEC);
#endif
      break;
      
      
      case set_request:
          pctime = atol(param);
#ifdef HW_CLOCK
          result = RTC.set(pctime);
#else
          result = processSyncMessage(pctime);
#endif

          if(result == 0)
             Serial.println("Success");
          else
            Serial.println("Failed");
      break;
    }
 
}
 
 
//======================================================================
// 
//====================================================================== 
void band_to_frequency()
{
       switch (bands)
       {
          case 160:
            freq = 1838000UL;
          break;

          case 80:
            freq = 3570000UL;
          break;
          case 60:
            freq = 5288600UL;
          break;

          case 40:
            freq = 7040000UL;
          break;

          case 30:
            freq = 10140100UL;
          break;

          case 20:
            freq = 14097000UL;
          break;

          case 17:
            freq = 18106000UL;
          break;

          case 15:
            freq = 21096000UL;
          break;

          case 12:
            freq = 24926000UL;
          break;

          case 10:
            freq = 28126000UL;
          break;

          default:
            freq = 10140100UL;
          
       }
}

 
#ifdef SI5351
void wspr_transmission()
{
//  Serial.println("Entering wspr_transmission");
//  Serial.println(freq);
//  Serial.println(tx_buffer[wspr_symbol]);


  if(wspr_state == on_air)
  {
     si5351.set_freq((freq * SI5351_FREQ_MULT) + (tx_buffer[wspr_symbol] * TONE_SPACING), SI5351_CLK0);

     if(++wspr_symbol == SYMBOL_COUNT)
     {
      // Turn off the output
        si5351.set_clock_pwr(SI5351_CLK0, 0);
        wspr_state = waiting_for_timeslot;
        digitalWrite(PTT_PIN, LOW);

      }
    else
        call_after(SYMBOL_TIME,  wspr_transmission);
  }
}
#endif

#ifdef AD9850DDS
void wspr_transmission()
{
  int phase = 0;
  
  if(wspr_state == on_air)
  {
    
    unsigned long TempWord;
    TempWord = FreqWord + OffsetFreq[tx_buffer[wspr_symbol]];
    ad9850_set_frequency(TempWord);
    if(++wspr_symbol == SYMBOL_COUNT)
    {
      // Turn off the output
      //TempWord=0;            // Turn off transmitter
      ad9850_set_frequency(0);
      ad9850_down();
      wspr_state = waiting_for_timeslot;
      digitalWrite(PTT_PIN, LOW);
    }
    else
        call_after(SYMBOL_TIME,  wspr_transmission);
  }
}
#endif

void start_wspr_transmission()
{
   char call[7]; 
   char loc[5];  
   uint8_t dbm;
   int calib_offset;
   int eeprom_address;
#ifdef AD9850DDS
   long unsigned int xtal_fq;
#endif
#ifdef SI5351
   int32_t cal_factor;
#endif
   switch(wspr_state)
   {
    case inactive:
    //Serial.println("Entering start_wspr_transmission in state inactive");
    break;

    case waiting_for_timeslot:
      eeprom_address = (int)offsetof(flash_layout, call);
      eestrget(eeprom_address, call, 8);

      eeprom_address = (int)offsetof(flash_layout, locator);
      eestrget(eeprom_address, loc, 5);

      eeprom_address = (int)offsetof(flash_layout, power);
      EEPROM.get(eeprom_address, dbm);
      jtencode.wspr_encode(call, loc, dbm, tx_buffer);

      EEPROM.get((int)offsetof(flash_layout,band),bands);
      
      eeprom_address = (int)offsetof(flash_layout, xtal_offset);
      EEPROM.get(eeprom_address, calib_offset);

      band_to_frequency();
    // Reset the tone to 0 and turn on the output
#ifdef SI5351
      cal_factor = -calib_offset;
      cal_factor = cal_factor * 100;
      si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
      si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
      si5351.set_clock_pwr(SI5351_CLK0, 1);
#endif
#ifdef AD9850DDS

      xtal_fq = fCLK - calib_offset;

      //Load offset values
      OffsetFreq[0] = 0;
      OffsetFreq[1] = 1.46*pow(2,32)/xtal_fq;
      OffsetFreq[2] = 2.93*pow(2,32)/xtal_fq;
      OffsetFreq[3] = 4.39*pow(2,32)/xtal_fq;
      ad9850_up();
      FreqWord = freq * pow(2,32)/xtal_fq;
#endif

      wspr_symbol = 0;
      wspr_state = on_air;
      call_after(1000,  wspr_transmission);
      digitalWrite(PTT_PIN, HIGH);
   break;

   case on_air:
   //Serial.println("Entering start_wspr_transmission in state on air");
   break;
   }
}




//======================================================================
// Send WSPR signal
//======================================================================
void wspr(char *command)
{
  char action[10];
  int power;
  //int bands;
  char *saveptr;
  int eeprom_address;
  char tmp_str[8];

  
  switch(request(command))
  {
     case set_request:
       strcpy(action, strtok_r(command, ",", &saveptr));
       if(strcmp(action,"TX") == 0)
       {
           eeprom_address = (int)offsetof(flash_layout, magic); 
           eestrget(eeprom_address, tmp_str, 4);
           if(strcmp(tmp_str, MAGIC)==0)
           {
           timeslot = atoi(strtok_r(NULL, ",", &saveptr));
           bands= atoi(strtok_r(NULL, ",", &saveptr));

       Serial.println("OK");
       wspr_state = waiting_for_timeslot;
       EEPROM.put((int)offsetof(flash_layout, state), wspr_state);
       EEPROM.put((int)offsetof(flash_layout,interval), timeslot);
       EEPROM.put((int)offsetof(flash_layout,band),bands);
       set_filter(bands);
       } //magic
       else
       Serial.println("NC");
       } // TX
       
       else
       // Cancel transmission
       if(strcmp(action,"CA") == 0)
       {
        Serial.println("stop");
#ifdef SI5351
        si5351.set_clock_pwr(SI5351_CLK0, 0);
#endif
#ifdef AD9850DDS
        //TempWord=0;            // Turn off transmitter
        ad9850_set_frequency(0);
        ad9850_down();
#endif
        set_filter(0);
        wspr_state = inactive;
        EEPROM.put((int)offsetof(flash_layout, state), wspr_state);
        }
        else
        // Status
        if(strcmp(action,"ST") == 0)
        {
//          Serial.println("Status");
        switch(wspr_state)
        {
        case inactive:
          Serial.print("DI");
           eeprom_address = (int)offsetof(flash_layout, magic); 
           eestrget(eeprom_address, tmp_str, 4);
           if(strcmp(tmp_str, MAGIC)==0)
           {
             EEPROM.get((int)offsetof(flash_layout, interval), timeslot);
             EEPROM.get((int)offsetof(flash_layout, band), bands);
             Serial.print(",");
             Serial.print(timeslot);
             Serial.print(",");
             Serial.println(bands);
          }
          else
             Serial.println("");
              
          break;

        case waiting_for_timeslot:
          Serial.print("WT,");
           Serial.print(timeslot);
          Serial.print(",");
          Serial.println(bands);
          break;

        case on_air:
          Serial.print("OA,");
          Serial.print(timeslot);
          Serial.print(",");
          Serial.println(bands);
          break;
          
        case tuning:
          Serial.print("TU,");
          Serial.print(timeslot);
          Serial.print(",");
          Serial.println(bands);
          break;
          
        }
  }

     break;
     
     case get_request:
          Serial.println("WS");

     break;
  }
 
} 
  
  

//======================================================================
// tune
//======================================================================
void tune(char *command)
{
  char *saveptr;
  int option;
  int eeprom_address;
  int calib_offset;
#ifdef AD9850DDS
   long unsigned int xtal_fq;
#endif
#ifdef SI5351
   int32_t cal_factor;

#endif
  switch(wspr_state)
  {
    case inactive:
      option = atoi(strtok_r(command, ",", &saveptr));
      bands = atoi(strtok_r(NULL, ",", &saveptr));

   
      eeprom_address = (int)offsetof(flash_layout, xtal_offset);
      EEPROM.get(eeprom_address, calib_offset);
      if(option == 2)
      {    
        band_to_frequency();
        set_filter(bands);
#ifdef SI5351
        cal_factor = -calib_offset;
        cal_factor=cal_factor*100;
        si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
        si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
        si5351.set_clock_pwr(SI5351_CLK0, 1);
        si5351.set_freq(freq * SI5351_FREQ_MULT, SI5351_CLK0);
#endif
#ifdef AD9850DDS
          xtal_fq = fCLK - calib_offset;
          ad9850_up();
          FreqWord = freq * pow(2,32)/xtal_fq;
          ad9850_set_frequency(FreqWord);    
#endif
        wspr_state = tuning;
         //Serial.print("TX");
         //Serial.print(option);
         //Serial.println(";");
         Serial.println(cal_factor);
      }
      else
      {
      Serial.print("TX");
      Serial.println(";");
      }
      break;

      default:
         Serial.println(";");

  } // switch
  
}

//======================================================================
// query_hw
//======================================================================
void query_hw(char *cmd)
{
    switch(request(cmd))
    {
        case get_request:
#ifdef AD9850DDS
            Serial.println("1");
#endif
#ifdef SI5351
            Serial.println("2");

#endif
        break;
        
        case set_request:
            Serial.println("ER");
        break;
    }
}

//======================================================================
//
//======================================================================
void error(char *cmd)
{
    int cmdi;
/*
    for(cmdi = 0; cmd[cmdi]; cmdi++)
    {
        if(log_write_pos < LOG_SIZE)
            EEPROM.write(cmdi, cmd[cmdi]);
    }
    EEPROM.write(cmdi, 0);
    */
    Serial.print("?;");
}

void parse_command(int status)
{
  int i=0;
//  Serial.print("String to be parsed: ");
//  Serial.println(commandbuffer);
  for(i = 0; strncmp(commandbuffer, cmd_table[i].cmd, 2) && strcmp(cmd_table[i].cmd, ""); i++);
    (*cmd_table[i].fun)(&commandbuffer[2]);
}

int poll_serial()
{
  if(Serial.available())
  {
    if(pos < BUFFER_SIZE){
      unsigned char ch = Serial.read();
      if(ch == ';') {
//        commandbuffer[pos++] = ch;
        commandbuffer[pos] = 0;
        pos = 0;
//        Serial.println("End of buffer found");
        parse_command(0);
      }
      else{
//        Serial.println("Adding ch");
        commandbuffer[pos++] = ch;
      };
    }
    else {
//      Serial.println("Buffer overflow");
      commandbuffer[pos] = 0;
      return(1); 
    };
  };
}


void setup()
{
  int eeprom_address;
  char tmp_str[10];
  
  
  
  pinMode(FILTER_80, OUTPUT);
  pinMode(FILTER_40, OUTPUT);
  pinMode(FILTER_20_17, OUTPUT);
  pinMode(FILTER_15_10, OUTPUT);

  
  
  
  Serial.begin(9600);

  // Set time sync provider
#ifdef HW_CLOCK
  setSyncProvider(RTC.get);  //set function to call when sync required
//#else  
//  setSyncProvider(requestSync);  //set function to call when sync required
#endif  

#ifdef SI5351
  // Initialize the Si5351
  // Change the 2nd parameter in init if using a ref osc other
  // than 25 MHz
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, CORRECTION);
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
#endif
#ifdef AD9850DDS
  ad9850_init(W_CLK_PIN, FQ_UD_PIN, S_DAT_PIN, RESET_PIN);
  ad9850_down();
#endif
  
  eeprom_address = (int)offsetof(flash_layout, magic); 
  eestrget(eeprom_address, tmp_str,5);
  if(strcmp(tmp_str, MAGIC)==0)
  {
      EEPROM.get((int)offsetof(flash_layout, state), wspr_state);
      EEPROM.get((int)offsetof(flash_layout,interval), timeslot);
      EEPROM.get((int)offsetof(flash_layout,band),bands);
  }
 
}
 
void loop()
{
  poll_serial();
  


  // WSPR should start on the 1st second of an even minute
  if( minute() % timeslot == 0 && second() == 0)
  {
    start_wspr_transmission();
  }
  poll_mstimer();
}
