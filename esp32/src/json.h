
#include "panglos/json.h"
//#include "panglos/drivers/gpio.h"
//#include "panglos/drivers/7-segment.h"

//void on_gpio(void *arg, panglos::json::Section *sec, panglos::json::Match::Type type);
//void on_display(void *arg, panglos::json::Section *sec, panglos::json::Match::Type type);

typedef void (*mqtt_match)(void *arg, panglos::json::Section *sec, panglos::json::Match::Type type, const char **keys);

void mqtt_subscribe(void *arg, mqtt_match fn, const char *info, bool verbose);
//void mqtt_gpio(panglos::GPIO *gpio, const char *info, bool verbose=false);
//void mqtt_display(panglos::SevenSegment *s, const char *info, bool verbose=false);

//  FIN
