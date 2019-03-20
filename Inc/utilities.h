#include "NeoSWSerial.h"

#define A7_PWR_KEY             10
#define BUFFER_SIZE           512
#define GPGGA_BUFFER           80
#define GPRMC_BUFFER           80
#define GPS_TIMES              10

#define AUTHENTICATION		    0

#define DEBUG                  	0
#define DEBUG_CMGL             	0
#define DEBUG_GPS              	0
#define DEBUG_SMS              	0


typedef enum {
	AUTH_OK,
	AUTH_ERROR,
	AUTH_PENDING
} auth_en;

typedef enum {
	A7_OK,
	A7_ERROR
} status_en;

void setup(void);
void loop(void);
void AT(int times);
void A7_init(int times);
void A7_io(void);
void A7_io_GPS(void);
void check_failure(int times);
void insert_pin(int times);
void query_network(int times);
void query_sim(int times);
void set_cmee(int times);
void disable_echo(int times);
void enable_echo(int times);
void set_clip(int times);
void set_sms_storage(int times);
void set_sms_format(int times);
void read_last_msg(int times);
void delete_all_saved_msgs(int times);
void check_callback(void);
void check_cmgl(int times);
void A7_SMS(int times, char *sms);
void A7_GPS_ON(int times);
void A7_GPSRD(int times);
void A7_GPS_OFF(int times);
void A7_CALL(int times);
void A7_REJECT_CALL(int times);
void callback_SMS (void);
void callback_GPS_ON (void);
void callback_GPS_OFF (void);
void callback_CALL (void);
void A7_gen_callback(void);
void reset_buffers(void);
void finalize_GPS_data(void);
void A7_GPS_NMEA_parse(int times);
void A7_GPS_extract_location (int times);
void A7_auth(void);
