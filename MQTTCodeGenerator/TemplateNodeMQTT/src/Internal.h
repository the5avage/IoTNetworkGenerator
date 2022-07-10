#include <PubSubClient.h>

extern bool connectionReady;
void subscribeToTopics(PubSubClient* client);
void initializeValues(PubSubClient* client);
void updateValues(char* topic, byte* message, unsigned int length);
extern const char *ssid;
extern const char *password;
extern const char *broker_address;
extern const char *client_name;
extern int broker_port;
