/********************************************************************************************** VERSION 8.2 WORKING FINAL **********************************************************************************************************/

#include "utilities.h"

NeoSWSerial A7_Serial(8,9);

char pin[] = "9212";
char phone_custom[] = "+306970133913";
//char incoming_number[20];


char rx_buffer[BUFFER_SIZE];

int gen_index = 0;

char recvd_sms_part1[] = "+CIEV:";
char recvd_sms_part2[] = "+CMT:";
char check_msg_sms[] = "OK!SMS!OK!";            // not to be finally implemented
char check_msg_gps_on[] = "OK!GPS*ON!OK!";
char check_msg_gps_off[] = "OK!GPS*OFF!OK!";
char check_msg_sms_call[] = "OK!CALL!OK!";      // not to be finally implemented
char check_msg_call[] = "+CLIP: \"";

// If the following two flags are set initially to 1, then the GPS datastreams need to be activated manually
// via call/sms action callback control command (i.e. call action or sms action containing "...OK!GPS*ON!OK!...").
// Otherwise, they start automatically.
int sms_flag1 = 1;
int sms_flag2 = 1;
int msgs_counter = 0;

char gps_NMEA_check_begin[] = "+GPSRD:";
int gps_NMEA_begin_lock = 0;
char gps_NMEA_check_GPGGA[] = "$GPGGA,";
int gps_NMEA_GPGGA_lock = 0;
char GPGGA[GPGGA_BUFFER];
char gps_NMEA_check_GPRMC[] = "$GPRMC,";
int gps_NMEA_GPRMC_lock = 0;
char GPRMC[GPRMC_BUFFER];
char gps_NMEA_check_GPVTG[] = "$GPVTG,";
int gps_NMEA_GPVTG_lock = 0;
int GPGGA_index = 0;
int GPRMC_index = 0;

int GPS_error = 0;

int GPS_iter = 0;

char char_in = '\0';

int i_GPS = 0;
int reset_buffers_lock = 1;

char sms[256];

auth_en auth = AUTH_PENDING;


void setup()
{

  /*********************************  BEGIN INITIALIZATION OF MODULE   ***************************************/

  // We have already saved (AT&W) the AT command AT+IPR=9600

  Serial.begin(9600);
  A7_Serial.begin(9600);


  pinMode(A7_PWR_KEY, OUTPUT);
  digitalWrite(A7_PWR_KEY, HIGH);
  delay(2100);
  digitalWrite(A7_PWR_KEY, LOW);

  delay(3500);
  
  //Clear out any waiting A7 serial data from the initialization procedure
  while (A7_Serial.available())
  {
    A7_Serial.read();
  }
  
  if (DEBUG)
    Serial.println(F("HERE01\r\n"));
    
  A7_init(1);

  reset_buffers();

  A7_io();
  delay(5000);

  /**********************************  END INITIALIZATION OF MODULE   ****************************************/
}





void loop()
{
  A7_io();
  check_callback();
  if ((sms_flag1 && sms_flag2) || (strstr(rx_buffer, check_msg_call) != NULL) \
                               || (strstr(rx_buffer, "BUSY") != NULL)         \
                               || (strstr(rx_buffer, "RING") != NULL)) {
      A7_gen_callback();
      
    if (DEBUG_CMGL)
      Serial.println(F("HERE404_1"));
  }


  /************************************   MAIN FUNCTION - PARSE AND PROCESS GPS DATA, SMS LOCATION LOGGING  *********************************/

  if (DEBUG_CMGL) {
    Serial.print(F("\r\n\nsms_flag1 : "));
    Serial.println(sms_flag1);
    Serial.print(F("\r\n\nsms_flag2 : "));
    Serial.println(sms_flag2);
    Serial.print(F("\r\n\nGPS_iter : "));
    Serial.println(GPS_iter);
  }
  if (!sms_flag1 || !sms_flag2) {    // OR because sms_flag1 may be 1 in the case of +CIEV when we have a call scenario.

    if (reset_buffers_lock) {
      if (DEBUG_CMGL)
        Serial.println(F("HERE404_3"));
        
      reset_buffers();
      reset_buffers_lock = 0;
    }
    
    A7_io();
    check_callback();
    if ((GPS_iter < GPS_TIMES) && (!sms_flag1 && !sms_flag2)) {
      if (DEBUG_GPS)
        Serial.println(F("GPS00"));
      
      A7_io();
      check_callback();
      if (!sms_flag1 && !sms_flag2) {
        A7_GPSRD(1);  // Oi polles fores tha ginoun mesw ths loop(0 kai den thelei orisma kati allo pera apo 1...
      }
      
      if (DEBUG_GPS)
        Serial.println(F("GPS03"));
      A7_GPS_NMEA_parse(1);
      if (DEBUG_GPS)
        Serial.println(F("GPS04"));
    }

    
    if (GPS_iter == GPS_TIMES) {

      sprintf(GPGGA, "$GPGGA,");
      sprintf(GPRMC, "$GPRMC,");
      A7_GPS_extract_location(1);
      
      if ((strstr(GPGGA, "0,00") != NULL) || (strstr(GPGGA, "N") == NULL)   \
                                          || (strstr(GPGGA, "E") == NULL)   \
                                          || (strstr(GPRMC, "V") != NULL))
        GPS_error = 1;
        

      if (!GPS_error) {
        
        if (DEBUG) {
          Serial.println(F("\r\n\nrx_buffer is : "));
          Serial.write(rx_buffer, BUFFER_SIZE);
          Serial.println(F("\r\n\n"));
        }
    
        if (DEBUG) {
          Serial.println(F("\r\n\nGPGGA is : "));
          Serial.write(GPGGA, strlen(GPGGA));
          Serial.println(F("\r\n\n"));
        }
    
        if (DEBUG) {
          Serial.println(F("\r\n\nGPRMC is : "));
          Serial.write(GPRMC, strlen(GPRMC));
          Serial.println(F("\r\n\n"));
        }
        sprintf(sms, "%s\r\n%s\r\n\n", GPGGA, GPRMC);
        
        A7_SMS(1, sms); // ENABLE THIS FOR SMS LOCATION LOGGING!
      }
           
        GPS_iter = 0;
        reset_buffers_lock = 1;
  
        memset(GPGGA, 0, GPGGA_BUFFER);
        memset(GPRMC, 0, GPRMC_BUFFER);
        GPGGA_index = 0;
        GPRMC_index = 0;
        reset_buffers();
        sms_flag1 = 0;
        sms_flag2 = 0;
        i_GPS = 0;
        GPS_error = 0;
    }
  }

  /******************************************************************************************************************************************/


  check_failure(1);
}





void AT(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT\r");
    delay(100);
  }
  delay(1000);
 if (DEBUG) 
    Serial.println(F("\r\nHERE03\r\n"));
}

void A7_init(int times) {
  AT(5);
  
  // Insert the PIN
  do {
    insert_pin(1);
    A7_io();
    if (DEBUG)
      Serial.println(F("\r\nPIN00\r\n"));
    query_network(1);
    A7_io();
    if (DEBUG)
      Serial.println(F("\r\nPIN01\r\n"));
    query_sim(1);
    A7_io();
    if (DEBUG)
      Serial.println(F("\r\nPIN02\r\n"));
    if (strstr(rx_buffer, "READY") != NULL)
      break;
    A7_io();
  } while ((strstr(rx_buffer, "+CREG: 1,13") != NULL) || (strstr(rx_buffer, "+CREG: 2") != NULL) \
                                                      || (strstr(rx_buffer, "+CREG: 3") != NULL) \
                                                      || (strstr(rx_buffer, "ERROR") != NULL)    \
                                                      || (strstr(rx_buffer, "SIM PIN") != NULL));

  if (DEBUG)
      Serial.println(F("\r\nPIN03\r\n"));
  if (DEBUG)
    Serial.println(F("\r\nHERE04\r\n"));
    
  set_cmee(1);
  if (DEBUG)
    Serial.println(F("\r\nHERE05a\r\n"));
    
  disable_echo(1);
//  enable_echo(1);
  if (DEBUG)
    Serial.println(F("\r\nHERE05b\r\n"));
    
  set_clip(1);
  if (DEBUG)
    Serial.println(F("\r\nHERE06\r\n"));
    
  set_sms_storage(1);
  if (DEBUG)
    Serial.println(F("\r\nHERE07\r\n"));
    
  set_sms_format(1);

  delete_all_saved_msgs(1);
  
  A7_GPS_ON(1);
  if (DEBUG)
    Serial.println(F("\r\nHERE09\r\n"));
}


void A7_io(void) {
  while (Serial.available())
  {
    A7_Serial.write(Serial.read());
  }

  while (A7_Serial.available())
  {
    char_in = A7_Serial.read();
    Serial.write(char_in);      
    rx_buffer[gen_index++] = char_in;
    if (gen_index >= BUFFER_SIZE)
      reset_buffers();
  }
}

void A7_io_GPS(void) {
  while(strstr(rx_buffer, "OK") == NULL) {
    while (A7_Serial.available())
    {
      char_in = A7_Serial.read();
      Serial.write(char_in);      
      rx_buffer[gen_index++] = char_in;
      if (gen_index >= BUFFER_SIZE)
        reset_buffers();
    }
  }
}

void check_failure(int times) {
  if ((strstr(rx_buffer, "CME ERROR") != NULL) || (strstr(rx_buffer, "SIM PIN") != NULL) || (strstr(rx_buffer, "not inserted") != NULL)) {
    delay(5000);
    AT(5);
    reset_buffers();
    GPS_iter = 0;
    i_GPS = 0;
  }
}

void insert_pin(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CPIN=9212\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE11\r\n"));
}

void query_network(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CREG?\r\n");
    delay(100);
  }
  delay(1000);
}

void query_sim(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CPIN?\r\n");
    delay(100);
  }
  delay(1000);
}

void set_cmee(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CMEE=2\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE10a\r\n"));
}

void disable_echo(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("ATE0\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE10b\r\n"));
}

void enable_echo(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("ATE1\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE10\r\n"));
}

void set_clip(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CLIP=1\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE12\r\n"));
}

void set_sms_storage(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE13\r\n"));
}

void set_sms_format(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CMGF=1\r\n");
    delay(100);
  }
  delay(1000);
  if (DEBUG)
    Serial.println(F("\r\nHERE14\r\n"));
}

void read_last_msg(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CMGR=");
    A7_Serial.write(msgs_counter);
    A7_Serial.write("\r\n");
    delay(100);
  }
  delay(1000);
}

void delete_all_saved_msgs(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+CMGD=1,4\r\n");
    delay(100);
  }
  delay(1000);
}


void check_callback(void) {
  if (sms_flag1 == 0) {
    if ((strstr(rx_buffer, recvd_sms_part1) != NULL) || (strstr(rx_buffer, check_msg_call) != NULL) \
                                                     || (strstr(rx_buffer, "RING") != NULL)) {
      if (DEBUG)
        Serial.println(F("HERE55"));
      sms_flag1 = 1;
    }
  }

  if (sms_flag2 == 0) {
    if ((strstr(rx_buffer, recvd_sms_part2) != NULL) || (strstr(rx_buffer, check_msg_call) != NULL) \
                                                     || (strstr(rx_buffer, "RING") != NULL)) {
      if (DEBUG)
        Serial.println(F("HERE56"));
      sms_flag2 = 1;
    }
  }

//  if (strstr(rx_buffer, "SMSFULL") != NULL)
//    delete_all_saved_msgs(1);
}

void check_cmgl(int times) {
  for (int i=0; i<times; ++i) {
    if ((strstr(rx_buffer, "+CMGL") != NULL) || (strstr(rx_buffer, "NO ANSWER") != NULL)) {
      if (DEBUG_CMGL)
        Serial.println(F("HERE404_00"));
      reset_buffers();
      GPS_iter = 0;
      i_GPS = 0;
      sms_flag1 = 0;
      sms_flag2 = 0;
      gps_NMEA_begin_lock = 0;
      gps_NMEA_GPGGA_lock = 0;
      gps_NMEA_GPRMC_lock = 0;
      gps_NMEA_GPVTG_lock = 0;
  
      if (DEBUG_CMGL)
        Serial.println(F("HERE404_01"));
    }
  }
}

void A7_SMS(int times, char *sms) {
  for (int i = 0; i < times; ++i) {
    //    A7_Serial.write("AT+CMGF=1\r\n");
    //    delay(2000);
    A7_Serial.write("AT+CMGS=\"");
    A7_Serial.write(phone_custom);
    A7_Serial.write("\"\r\n");
    delay(2000);
    A7_Serial.write(sms);
    delay(500);
    A7_Serial.write (char(26));//the ASCII code of the ctrl+z is 26
  }
}

void A7_GPS_ON(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("AT+GPS=1\r\n");
    delay(1000);
  }
}

void A7_GPSRD(int times) {
  for (int i = 0; i < times; ++i) {
    if (DEBUG_GPS)
        Serial.println(F("GPS01"));
    A7_Serial.write("AT+GPSRD\r\n");
    if (DEBUG_GPS)
        Serial.println(F("GPS02"));
    A7_io_GPS();
    delay(1000);

    A7_io();
    check_callback();
  }
}

void A7_GPS_OFF(int times) {
  for (int i = 0; i < times; ++i) {
    A7_Serial.write("\r\n\nAT+GPS=0\r\n\n");
    delay(1000);
  }
}

void A7_CALL(int times) {
  for (int i = 0; i < times; ++i) {
    delay(2000);
    A7_Serial.write("ATD");
    A7_Serial.write(phone_custom);
    A7_Serial.write(";\r\n");
    delay(2000);
  }
}

void A7_REJECT_CALL(int times) {
  for (int i = 0; i < times; ++i) {
    delay(2000);
    A7_Serial.write("ATH\r\n\n");
  }
}

void callback_SMS (void) {
  char custom_sms[] = "Action callback from A7...";
  A7_SMS(1, custom_sms);
}

void callback_GPS_ON (void) {
//  A7_GPS_ON(1);
//  A7_GPSRD(GPS_TIMES);
// Enabling the conditions in order to start AT+GPSRD in loop():
  sms_flag1 = 0;
  sms_flag2 = 0;
}

void callback_GPS_OFF (void) {
//  A7_GPS_ON(1);
//  A7_GPSRD(GPS_TIMES);
// Disabling the conditions in order to stop AT+GPSRD sthn loop():
  sms_flag1 = 1;
  sms_flag2 = 1;
}

void callback_CALL (void) {
  if (DEBUG)
    Serial.println(F("HERE57"));
  A7_CALL(1);
}

void A7_gen_callback(void) {
  if (AUTHENTICATION)
        A7_auth();
  if (strstr(rx_buffer, check_msg_sms) != NULL) {
    if (AUTHENTICATION) {
      if (auth==AUTH_OK) {
        if (DEBUG_SMS)
          Serial.println(F("SMS_00"));
        sms_flag1 = 0;
        sms_flag2 = 0;
        gen_index = 0;
        reset_buffers();
        GPS_iter = 0;
        i_GPS = 0;
        gps_NMEA_begin_lock = 0;
        gps_NMEA_GPGGA_lock = 0;
        gps_NMEA_GPRMC_lock = 0;
        gps_NMEA_GPVTG_lock = 0;
    
        if (DEBUG)
          Serial.println(F("HERE50"));
    
        callback_SMS();
      }
      else if (auth==AUTH_ERROR) {}
    }
    else {
      sms_flag1 = 0;
      sms_flag2 = 0;
      gen_index = 0;
      reset_buffers();
      GPS_iter = 0;
      i_GPS = 0;
      gps_NMEA_begin_lock = 0;
      gps_NMEA_GPGGA_lock = 0;
      gps_NMEA_GPRMC_lock = 0;
      gps_NMEA_GPVTG_lock = 0;
      callback_SMS();
    }
  }

  if (strstr(rx_buffer, check_msg_gps_on) != NULL) {
    if (AUTHENTICATION) {
      if (auth==AUTH_OK) {
        sms_flag1 = 0;
        sms_flag2 = 0;
        gen_index = 0;
        reset_buffers();
        GPS_iter = 0;
        i_GPS = 0;
        gps_NMEA_begin_lock = 0;
        gps_NMEA_GPGGA_lock = 0;
        gps_NMEA_GPRMC_lock = 0;
        gps_NMEA_GPVTG_lock = 0;
    
        if (DEBUG)
          Serial.println(F("HERE51a"));
      
        callback_GPS_ON();
      }
      else if (auth==AUTH_ERROR) {}
    }
    else {
      sms_flag1 = 0;
      sms_flag2 = 0;
      gen_index = 0;
      reset_buffers();
      GPS_iter = 0;
      i_GPS = 0;
      gps_NMEA_begin_lock = 0;
      gps_NMEA_GPGGA_lock = 0;
      gps_NMEA_GPRMC_lock = 0;
      gps_NMEA_GPVTG_lock = 0;
      callback_GPS_ON();
    }
  }

  if (strstr(rx_buffer, check_msg_gps_off) != NULL) {
    if (AUTHENTICATION) {
      if (auth==AUTH_OK) {
        sms_flag1 = 0;
        sms_flag2 = 0;
        gen_index = 0;
        reset_buffers();
        GPS_iter = 0;
        i_GPS = 0;
        gps_NMEA_begin_lock = 0;
        gps_NMEA_GPGGA_lock = 0;
        gps_NMEA_GPRMC_lock = 0;
        gps_NMEA_GPVTG_lock = 0;
    
        if (DEBUG)
          Serial.println(F("HERE51b"));
    
        callback_GPS_OFF();
      }
      else if (auth==AUTH_ERROR) {}
    }
    else {
      sms_flag1 = 0;
      sms_flag2 = 0;
      gen_index = 0;
      reset_buffers();
      GPS_iter = 0;
      i_GPS = 0;
      gps_NMEA_begin_lock = 0;
      gps_NMEA_GPGGA_lock = 0;
      gps_NMEA_GPRMC_lock = 0;
      gps_NMEA_GPVTG_lock = 0;
      callback_GPS_OFF();
    }
  }

  if (strstr(rx_buffer, check_msg_call) != NULL) {
    if (AUTHENTICATION) {
      if (auth==AUTH_OK) {
        A7_REJECT_CALL(1);
        delay(2000);
        sms_flag1 = 0;
        sms_flag2 = 0;
        gen_index = 0;
        reset_buffers();
        GPS_iter = 0;
        i_GPS = 0;
        gps_NMEA_begin_lock = 0;
        gps_NMEA_GPGGA_lock = 0;
        gps_NMEA_GPRMC_lock = 0;
        gps_NMEA_GPVTG_lock = 0;
        A7_CALL(1);
      }
      else if (auth==AUTH_ERROR) {}
    }
    else {
      A7_REJECT_CALL(1);
      delay(2000);
      sms_flag1 = 0;
      sms_flag2 = 0;
      gen_index = 0;
      reset_buffers();
      GPS_iter = 0;
      i_GPS = 0;
      gps_NMEA_begin_lock = 0;
      gps_NMEA_GPGGA_lock = 0;
      gps_NMEA_GPRMC_lock = 0;
      gps_NMEA_GPVTG_lock = 0;
      A7_CALL(1);
    }
  }

  if (strstr(rx_buffer, "BUSY") != NULL) {
    sms_flag1 = 0;
    sms_flag2 = 0;
    gen_index = 0;
    reset_buffers();
    GPS_iter = 0;
    i_GPS = 0;
    gps_NMEA_begin_lock = 0;
    gps_NMEA_GPGGA_lock = 0;
    gps_NMEA_GPRMC_lock = 0;
    gps_NMEA_GPVTG_lock = 0;
    
    if (DEBUG)
      Serial.println(F("HERE53"));
  }
}

void reset_buffers(void) {
  memset(rx_buffer, 0, BUFFER_SIZE);
  gen_index = 0; 
}

void finalize_GPS_data(void) {
  gps_NMEA_begin_lock = 0;
  gps_NMEA_GPGGA_lock = 0;
  gps_NMEA_GPRMC_lock = 0;
  gps_NMEA_GPVTG_lock = 0;

  GPS_iter++;
  if (DEBUG)
      Serial.println(F("HERE24\n"));

  if (GPS_iter < GPS_TIMES) {
    if (strstr(rx_buffer, check_msg_call) == NULL)
      reset_buffers();
  }
}

void A7_GPS_NMEA_parse(int times) {

  for (int i = 0; i < times; ++i)  {
    check_callback();
    if (!gps_NMEA_begin_lock) {     
      if (strstr(rx_buffer, gps_NMEA_check_begin) != NULL) {
        gps_NMEA_begin_lock = 1;
        gps_NMEA_GPGGA_lock = 0;
        gps_NMEA_GPRMC_lock = 0;
        gps_NMEA_GPVTG_lock = 0;

        if (DEBUG)
          Serial.println(F("HERE20\n"));
      }
    }

    
    if (gps_NMEA_begin_lock && !gps_NMEA_GPGGA_lock) {
      A7_io();

      if (DEBUG_CMGL)
        Serial.println(F("HERE404_6"));

      if ((strstr(rx_buffer, gps_NMEA_check_GPGGA)) != NULL) {
        gps_NMEA_GPGGA_lock = 1;

        if (DEBUG_CMGL)
          Serial.println(F("HERE404_7"));
        if (DEBUG)
            Serial.println(F("HERE21\n"));
      }
    }

    if (gps_NMEA_begin_lock && !gps_NMEA_GPRMC_lock) {
      A7_io();

       if (DEBUG_CMGL)
        Serial.println(F("HERE404_8"));
      

      if ((strstr(rx_buffer, gps_NMEA_check_GPRMC)) != NULL) {
        gps_NMEA_GPRMC_lock = 1;
        
        if (DEBUG_CMGL)
          Serial.println(F("HERE404_9"));
        if (DEBUG)
            Serial.println(F("HERE22\n"));
      }
    }


    if (gps_NMEA_begin_lock && !gps_NMEA_GPVTG_lock) {
      A7_io();

      if (DEBUG_CMGL)
        Serial.println(F("HERE404_10"));
      char *ret;
      if (((ret = strstr(rx_buffer, gps_NMEA_check_GPVTG)) != NULL) && (strstr(ret, "OK") != NULL)) {

        gps_NMEA_GPVTG_lock = 1;
        
        if (DEBUG_CMGL)
          Serial.println(F("HERE404_11"));
        if (DEBUG)
            Serial.println(F("HERE23\n"));
      }
    }
    
    
    if (gps_NMEA_begin_lock && gps_NMEA_GPGGA_lock && gps_NMEA_GPRMC_lock && gps_NMEA_GPVTG_lock) {
      finalize_GPS_data();
      if (DEBUG_CMGL)
        Serial.println(F("HERE404_12"));
    }
  }
}

void A7_GPS_extract_location (int times) {
  char *ret;
  if ((ret = strstr(rx_buffer, gps_NMEA_check_GPGGA)) != NULL) {
    ret += 7;                         // 7 is the length of the string "$GPGGA," -> read from the 7th position! (0-6 reserved)
    while (*ret != '$') {
      GPGGA[GPGGA_index + 7] = *ret;  // 7 is the length of the string "$GPGGA," -> write to the 7th position! (0-6 reserved)
      GPGGA_index++;
      ret++;
    }
    
    if (DEBUG)
        Serial.println(F("HERE21\n"));
  }
  
  if ((ret = strstr(rx_buffer, gps_NMEA_check_GPRMC)) != NULL) {
    ret += 7;                         // 7 is the length of the string "$GPRMC," -> read from the 7th position! (0-6 reserved)
    while (*ret != '$') {
      GPRMC[GPRMC_index + 7] = *ret;  // 7 is the length of the string "$GPRMC," -> write to the 7th position! (0-6 reserved)
      GPRMC_index++;
      ret++;
    }
    
    if (DEBUG)
        Serial.println(F("HERE22\n"));
  }
}

void A7_auth(void) {
  if ((strstr(rx_buffer, phone_custom) != NULL) || (strstr(rx_buffer, &phone_custom[1]) != NULL))
    auth = AUTH_OK;
  else
    auth = AUTH_ERROR;
}

//void A7_get_incoming_number_NOT_TO_BE_USED(void) {
//  char *ret_start, *ret_end;
//  A7_io();
////  Serial.println(F("rx_buffer : "));
////  Serial.println(rx_buffer);
//  if (strstr(rx_buffer, "+CLIP: \"") != NULL) {
//    ret_start = strstr(rx_buffer, "+CLIP: \"");
//    ret_start += 8;                              // 8 is the size of <+CLIP: ">
//    ret_end = strstr(ret_start, "\"");
//    *ret_end = '\0';
//    sprintf(incoming_number, "+%s", ret_start); // the +CLIP contains the incoming number in the format:
//                                                // <306970133913>, so we need to add a <+> in the beginning.
//  }
//  else {
//    ret_start = strstr(rx_buffer, "+CMT: \"");
//    ret_start += 7;                             // 7 is the size of <+CMT: ">
//    ret_end = strstr(ret_start, "\"");
//    *ret_end = '\0';
//    sprintf(incoming_number, "%s", ret_start);  // the +CMT contains the incoming number in the format:
//                                                // <+306970133913>, so we don't need to add a <+> in the beginning.
//  }
//  
//  Serial.println(F("incoming_number  : "));
//  Serial.println(incoming_number);
//
//  A7_auth();
//}
//
//void A7_auth_NOT_TO_BE_USED(void) {
//  if (!strcmp(incoming_number, phone_custom))
//    auth = AUTH_OK;
//  else
//    auth = AUTH_ERROR;
//}

