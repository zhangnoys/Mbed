#include "mbed.h"
#include "SpwfInterface.h"
#include "TCPSocket.h"
#include "MQTTClient.h"
#include "MQTTWiFi.h"
#include <ctype.h>
#include "NDefLib/NDefNfcTag.h"
#include "NDefLib/RecordType/RecordURI.h"
#include "Servo.h"
#include"XNucleoIKS01A2.h"
#include "stm32f4xx_hal.h"
Servo servo(D10);

float range = 0.005;
float position = 0.5;

Serial pc(SERIAL_TX, SERIAL_RX); 
bool quickstartMode = true;    

#define ORG_QUICKSTART   101.132.47.170        // comment to connect to play.internetofthings.ibmcloud.com
//#define SUBSCRIBE              // uncomment to subscribe to broker msgs (not to be used with IBM broker) 
    
#define MQTT_MAX_PACKET_SIZE 250   
#define MQTT_MAX_PAYLOAD_SIZE 300 

 // Configuration values needed to connect to IBM IoT Cloud
#define BROKER_URL ".132.47.170";     
#ifdef ORG_QUICKSTART
#define ORG "101"     // connect to quickstart.internetofthings.ibmcloud.com/ For a registered connection, replace with your org 
#define ID "168"
#define AUTH_TOKEN "10"
#define DEFAULT_TYPE_NAME "145"
#else   // not def ORG_QUICKSTART
#define ORG "192"             // connect to play.internetofthings.ibmcloud.com/ For a registered connection, replace with your org
#define ID "168"       // For a registered connection, replace with your id
#define AUTH_TOKEN "10"// For a registered connection, replace with your auth-token
#define DEFAULT_TYPE_NAME "145"
#endif
#define TOPIC  "TempA" 
#define TOPIC2  "commond" 
#define  TOPICT_hum  "hum" 
#define TOPICT_x "x"
#define TOPICT_y "y"
#define TOPICT_z "z"
#define  TOPICT_ax  "a_x" 
#define  TOPICT_ay  "a_y" 
#define  TOPICT_az  "a_z" 
#define  TOPICT_bx  "b_x" 
#define  TOPICT_by  "b_y" 
#define  TOPICT_bz  "b_z" 


#define TYPE DEFAULT_TYPE_NAME       // For a registered connection, replace with your type
#define MQTT_PORT 1883
#define MQTT_TLS_PORT 8883
#define IBM_IOT_PORT MQTT_PORT
// WiFi network credential
#define SSID   "LongFreeMoble"  // Network must be visible otherwise it can't connect
#define PASSW  "iots8230"
#warning "Wifi SSID & password empty"
char* topic = "TempA";
    
char id[30] = ID;                 // mac without colons  
char org[12] = ORG;        
int connack_rc = 0; // MQTT connack return code
const char* ip_addr = "";
char* host_addr = "";
char type[30] = TYPE;
char auth_token[30] = AUTH_TOKEN; // Auth_token is only used in non-quickstart mode
bool netConnecting = false;
int connectTimeout = 1000;
bool mqttConnecting = false;
bool netConnected = false;
bool connected = false;
int retryAttempt = 0;
char subscription_url[MQTT_MAX_PAYLOAD_SIZE];
     
     
     static XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(D14, D15, D4, D5);

    /* Retrieve the composing elements of the expansion board */
    static LSM303AGRMagSensor *magnetometer = mems_expansion_board->magnetometer;
    static HTS221Sensor *hum_temp = mems_expansion_board->ht_sensor;
    static LPS22HBSensor *press_temp = mems_expansion_board->pt_sensor;
    static LSM6DSLSensor *acc_gyro = mems_expansion_board->acc_gyro;
    static LSM303AGRAccSensor *accelerometer = mems_expansion_board->accelerometer;

class WatchDog {
 
private:
 
    IWDG_HandleTypeDef hiwdg;
 
public:
 
    WatchDog(uint32_t prescaler = IWDG_PRESCALER_256, uint32_t reload = 0xfff) {
 
        hiwdg.Instance = IWDG;
 
        hiwdg.Init.Prescaler = prescaler;
 
        hiwdg.Init.Reload = reload;
 
        HAL_IWDG_Init(&hiwdg);
 
    }
 
    void feed() {
 
        HAL_IWDG_Refresh(&hiwdg);
 
    }
 
};


/* Retrieve the composing elements of the expansion board */
//PressureSensor *pressure_sensor;
//HumiditySensor *humidity_sensor;
//TempSensor *temp_sensor1;

MQTT::Message message;
MQTTString TopicName={TOPIC};
MQTT::MessageData MsgData(TopicName, message);

void subscribe_cb(MQTT::MessageData & msgMQTT) {
    char msg[MQTT_MAX_PAYLOAD_SIZE];
    msg[0]='\0';
    strncat (msg, (char*)msgMQTT.message.payload, msgMQTT.message.payloadlen);
    printf ("--->>> subscribe_cb msg: %s\n\r", msg);
    printf("%d\r\n",strlen(msg));
    if(strlen(msg)==2) 
    {
        printf ("I AM WORKING\n\r");
        int i=0;
        while(i<500) {
            i++;
            servo = position;
            position += range;
            if(position >= 1) {
                position = 0;
                wait(0.5);
            }
            wait(0.002);
        }
    }
}

int subscribe(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    char* pubTopic = TOPIC2;    
    printf("subscribe int finished\r\n");
    return client->subscribe(pubTopic, MQTT::QOS1, subscribe_cb);
}

int connect(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{ 
    const char* iot_ibm = BROKER_URL; 

    
    char hostname[strlen(org) + strlen(iot_ibm) + 1];
    sprintf(hostname, "%s%s", org, iot_ibm);
    SpwfSAInterface& WiFi = ipstack->getWiFi();
    char clientId[strlen(org) + strlen(type) + strlen(id) + 5];  
    sprintf(clientId, "d:%s:%s:%s", org, type, id);  
    sprintf(subscription_url, "%s.%s/#/device/%s/sensor/", org, "iots",id);

    // Network debug statements 
    LOG("\n\r=====================================\n\r");
    LOG("Connecting WiFi.\n\r");
    LOG("Nucleo IP ADDRESS: %s\n\r", WiFi.get_ip_address());
    LOG("Nucleo MAC ADDRESS: %s\n\r", WiFi.get_mac_address());
    LOG("Server Hostname: %s port: %d\n\r", hostname, IBM_IOT_PORT);
    LOG("Client ID: %s\n\r", clientId);
    LOG("Topic: %s\n\r",TOPIC);
    LOG("Subscription URL: %s\n\r", subscription_url);
    LOG("=====================================\n\r");
    
    netConnecting = true;
    ipstack->open(&ipstack->getWiFi());
    int rc = ipstack->connect(hostname, IBM_IOT_PORT, connectTimeout);    
    if (rc != 0)
    {
        WARN("IP Stack connect returned: %d\n", rc);    
        return rc;
    }
    printf ("--->TCP Connected\n\r");
    netConnected = true;
    netConnecting = false;

    // MQTT Connect
    mqttConnecting = true;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.struct_version=0;
    data.clientID.cstring = clientId;
 
    if (!quickstartMode) 
    {        
        data.username.cstring = "use-token-auth";
        data.password.cstring = auth_token;
    }   
    if ((rc = client->connect(data)) == 0) 
    {       
        connected = true;
        printf ("--->MQTT Connected\n\r");
#ifdef SUBSCRIBE
        if (!subscribe(client, ipstack)) printf ("--->>>MQTT subscribed to: %s\n\r",TOPIC);
#endif           
    }
    else {
        WARN("MQTT connect returned %d\n", rc);        
    }
    if (rc >= 0)
        connack_rc = rc;
    mqttConnecting = false;
    return rc;
}

int getConnTimeout(int attemptNumber)
{  // First 10 attempts try within 3 seconds, next 10 attempts retry after every 1 minute
   // after 20 attempts, retry every 10 minutes
    return (attemptNumber < 10) ? 3 : (attemptNumber < 20) ? 60 : 600;
}

void attemptConnect(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    connected = false;
           
    while (connect(client, ipstack) != MQTT_CONNECTION_ACCEPTED) 
    {    
        if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD) {
            printf ("File: %s, Line: %d Error: %d\n\r",__FILE__,__LINE__, connack_rc);        
            return; // don't reattempt to connect if credentials are wrong
        } 
        int timeout = getConnTimeout(++retryAttempt);
        WARN("Retry attempt number %d waiting %d\n", retryAttempt, timeout);
        
        // if ipstack and client were on the heap we could deconstruct and goto a label where they are constructed
        //  or maybe just add the proper members to do this disconnect and call attemptConnect(...)        
        // this works - reset the system when the retry count gets to a threshold
        if (retryAttempt == 5)
            NVIC_SystemReset();
        else
            wait(timeout);
    }
}

int publish(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    MQTT::Message message;
    char* pubTopic = TOPIC;
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    float temp, press, hum;
    printf("%f\r\n",press);
 hum_temp->get_temperature(&temp);
    hum_temp->get_humidity(&hum);
 //   temp_sensor1->GetTemperature(&temp);
 //   humidity_sensor->GetHumidity(&hum);
 //   pressure_sensor->GetPressure(&press);
  
  
    sprintf(buf,"%0.4f",temp);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    //message.payloadlen = sizeof(temp);
    
//    LOG("Publishing %s\n\r", buf);
    printf("Publishing hum %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish2(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    MQTT::Message message;
    char* pubTopic = TOPICT_hum;
    uint8_t id;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    float temp, press, hum;
    printf("%f\r\n",press);
     hum_temp->get_temperature(&temp);
    hum_temp->get_humidity(&hum);
   // temp_sensor1->GetTemperature(&temp);
  //  humidity_sensor->GetHumidity(&hum);
 //   pressure_sensor->GetPressure(&press);
  
  
    sprintf(buf,"%0.4f",hum);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    //message.payloadlen = sizeof(temp);
    
//    LOG("Publishing %s\n\r", buf);
    printf("Publishing temp %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish3(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
     int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_x;
    uint8_t id;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    float temp, press, hum;
    printf("%f\r\n",press);
     hum_temp->get_temperature(&temp);
    hum_temp->get_humidity(&hum);
    magnetometer->get_m_axes(axes);
  
  
    sprintf(buf,"%0.4d",axes[0]);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing x %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish4(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
     int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_y;
    uint8_t id;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    magnetometer->get_m_axes(axes);
    
    printf("axes's number%d\r\n",axes[1]);
    sprintf(buf,"%0.4d",axes[1]);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing y %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish5(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
     int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_z;
    uint8_t id;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    float temp, press, hum;
     hum_temp->get_temperature(&temp);
    hum_temp->get_humidity(&hum);
    magnetometer->get_m_axes(axes);
  
  
    sprintf(buf,"%0.4d",axes[2]);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    //message.payloadlen = sizeof(temp);
    
//    LOG("Publishing %s\n\r", buf);
    printf("Publishing z %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish7(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_ax;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    accelerometer->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[0]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish8(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_ay;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    accelerometer->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[1]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish9(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_az;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    accelerometer->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[2]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish10(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_bx;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    
    acc_gyro->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[0]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish11(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_by;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    
    acc_gyro->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[1]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
int publish12(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    int32_t axes[3];
    MQTT::Message message;
    char* pubTopic = TOPICT_bz;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    
    acc_gyro->get_x_axes(axes);
     sprintf(buf,"%0.4d",axes[2]);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
    
    
int main()
{
  
    const char * ssid = SSID; // Network must be visible otherwise it can't connect
    const char * seckey = PASSW;
    SpwfSAInterface spwf(D8, D2, false);
    DevI2C *i2c = new DevI2C(I2C_SDA, I2C_SCL);
    i2c->frequency(400000);    
    
    
 
    
    pc.printf("\r\nX-NUCLEO-IDW01M1 mbed Application\r\n");     
    pc.printf("\r\nconnecting to AP\r\n");            
        
  hum_temp->enable();
  press_temp->enable();
  magnetometer->enable();
  accelerometer->enable();
  acc_gyro->enable_x();
  acc_gyro->enable_g();

   quickstartMode=false;
   if (strcmp(org, "quickstart") == 0){quickstartMode = true;}
   MQTTWiFi ipstack(spwf, ssid, seckey, NSAPI_SECURITY_WPA2);
   MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);
   if (quickstartMode){
        char mac[50];  // remove all : from mac
        char *digit=NULL;
        sprintf (id,"%s", "");                
        sprintf (mac,"%s",ipstack.getWiFi().get_mac_address()); 
        digit = strtok (mac,":");
        while (digit != NULL)
        {
            strcat (id, digit);
            digit = strtok (NULL, ":");
        }     
   }
   attemptConnect(&client, &ipstack);
   if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD)    
   {
      while (true)
      wait(1.0); // Permanent failures - don't retry
   }    
    WatchDog wdg(IWDG_PRESCALER_64, 62500);  
   int count = 0;    
//    tyeld.start();    
    while (true)
    {   
     wdg.feed();   
    if (++count == 100)
        {  
        if(subscribe(&client, &ipstack)>30){
            printf("ok \n\r");
            }         
    publish2(&client, &ipstack);
    publish3(&client, &ipstack);
    publish4(&client, &ipstack);
    publish5(&client, &ipstack);
    publish5(&client, &ipstack);
    publish7(&client, &ipstack);
      publish8(&client, &ipstack);
        publish9(&client, &ipstack);
        publish10(&client, &ipstack);
        publish11(&client, &ipstack);
        publish12(&client, &ipstack);
          if (publish(&client, &ipstack) != 0) { 
         attemptConnect(&client, &ipstack);   // if we have lost the connection                
           }
          count = 0;
        }        
        client.yield(10);
     }
}
