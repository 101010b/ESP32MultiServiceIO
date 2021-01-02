#ifndef __MULTISERVICE_IO_H__
#define __MULTISERVICE_IO_H__

#include <Arduino.h>
#include <stdint.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLEDevice.h>
#include <BLEUUID.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <BLE2904.h>

#include "config.h"

typedef enum
{
    ui32 = 0,
    i32,
    f32,
    str,
    LAST    
} VARTYPE;

#define TOPICLEN 64
// const int VARLEN[] = {4,4,4,8,1}; // Must match the VARTYPES above!

typedef enum
{
    UNINITIALIZED = 0,
    OFFLINE,
    ONLINE,
    MQTTDOCONNECT,
    MQTTCONNECTED
} WIFISTATE;

typedef struct
{
    uint8_t changeflag;
    uint32_t min;
    uint32_t max;
    uint32_t val;
} UI32Var_t;

typedef struct
{
    uint8_t changeflag;
    int32_t min;
    int32_t max;
    int32_t val;
} I32Var_t;

#define X32Set(X,mi,va,ma) (X).min=mi;(X).max=ma;(X).val=va;(X).changeflag=0;
#define STRSet(X,val) strcpy((X).str, (val)); (X).changeflag=0;

typedef struct {
    uint8_t changeflag;
    uint8_t unknown[0];
} ANYVAR_t;

typedef struct
{
    uint8_t changeflag;
    float min;
    float max;
    float val;
} F32Var_t;

typedef struct 
{
    uint8_t changeflag;
    char str[0];
} S0Var_t;

typedef struct
{
    uint8_t changeflag;
    char str[16 + 1];
} S16Var_t;

typedef struct
{
    uint8_t changeflag;
    char str[32 + 1];
} S32Var_t;

typedef struct
{
    uint8_t changeflag;
    char str[64 + 1];
} S64Var_t;

typedef union
{
    ANYVAR_t any;
    UI32Var_t ui32;
    I32Var_t i32;
    F32Var_t f32;
    S0Var_t str;
} GENERIC_t;

typedef union 
{
    uint32_t ui32;
    int32_t i32;
    float f32;
} X32_t;

// Flags
#define FLAG_SYSTEM         (0x01)
#define FLAG_PASSWORD       (0x02)
#define FLAG_COLOR          (0x80)
#define ISSYSTEM(cfg)       (((cfg)->flags) & FLAG_SYSTEM)
#define SETSYSTEM(cfg)      (cfg)->flags |= FLAG_SYSTEM
#define ISPASSWORD(cfg)     (((cfg)->flags) & FLAG_PASSWORD)
#define SETPASSWORD(cfg)    (cfg)->flags |= FLAG_PASSWORD
#define ISCOLOR(cfg)        (((cfg)->flags) & FLAG_COLOR)
#define SETCOLOR(cfg)       (cfg)->flags |= FLAG_COLOR
#define CLEARCOLOR(cfg)     (cfg)->flags &= ~FLAG_COLOR

// Color Conversion Macros
#define RGB(r, g, b) ((((uint32_t)r) << 16) | (((uint32_t)g) << 8) | ((uint32_t)b))
#define R(u) ((uint8_t)(((u) >> 16) & 0x000000FF))
#define G(u) ((uint8_t)(((u) >> 8) & 0x000000FF))
#define B(u) ((uint8_t)((u)&0x000000FF))

// ChangeFlags
#define FLAG_LOCAL          (0x01)
#define FLAG_BLE            (0x02)
#define FLAG_MQTT           (0x04)

#define ISLOCAL(var) ((var).changeflag & FLAG_LOCAL)
#define SETLOCAL(var) (var).changeflag |= FLAG_LOCAL
#define CLEARLOCAL(var) (var).changeflag &= ~FLAG_LOCAL

#define ISBLE(var) ((var).changeflag & FLAG_BLE)
#define SETBLE(var) (var).changeflag |= FLAG_BLE
#define CLEARBLE(var) (var).changeflag &= ~FLAG_BLE

#define ISMQTT(var) ((var).changeflag & FLAG_MQTT)
#define SETMQTT(var) (var).changeflag |= FLAG_MQTT
#define CLEARMQTT(var) (var).changeflag &= ~FLAG_MQTT

// Be careful changing this number!!! It will mess up BLE Packages
#define MAXNAMEMELN 32

typedef struct ConfigValue_s
{
    struct ConfigValue_s *next;
    BLECharacteristic *bleChar; // Corresponding BLE Characteristic
    uint16_t storelen; // in bytes
    char name[MAXNAMEMELN+1];
    VARTYPE vartype;
    uint8_t flags;
    GENERIC_t *val;
} ConfigValue_t;

class MultiServiceIO;

class bleCharacteristicCallback : public BLECharacteristicCallbacks
{
private:
    void onWrite(BLECharacteristic *pCharacteristic);
    void onRead(BLECharacteristic *pCharacteristic);
public:
    MultiServiceIO *m = NULL;
};

class bleServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
public:
    MultiServiceIO *m = NULL;
};

typedef struct
{
    uint16_t packetID;
    uint8_t data[18];
} __attribute__((packed)) notify_t;

class MultiServiceIO {
    public: // Variables
        BLEServer *bleServer = NULL;
        bool BLEConnected = false;
        bool BLEUnlocked = false;
        BLEAdvertising *pAdvertising = NULL;
        BLEService *devInfoSvc = NULL;
        BLEService *cfgSvc = NULL;
        BLEService *prmSvc = NULL;
        bleCharacteristicCallback blecbk;
        bleServerCallbacks blesrvcbk;
        bool notifyTX=false;
        int16_t packetsTX=0;
        int16_t packstart, packstop;
        notify_t notify;
        BLECharacteristic *txChar;

        bool WiFiConnected = false;
        void mqttCallback(const char *topic, unsigned char *payload, unsigned int length);
        void pushU8(uint32_t *idx, uint32_t first, uint32_t last, uint8_t data);
        void push(uint32_t *idx, uint32_t first, uint32_t last, uint8_t *data, uint32_t len);
        bool buildTXPackage(uint16_t packet);
        void BLEDisconnect();
        void BLEConnect();
        void BLEUnlock();

    public: // Functions
        // Public Variable creation Functions
        void addNewConfigValue(const char *name, UI32Var_t *ui32, bool asColor=false);
        void addNewConfigValue(const char *name, I32Var_t *i32);
        void addNewConfigValue(const char *name, F32Var_t *f32);
        void addNewConfigValue(const char *name, S16Var_t *s16);
        void addNewConfigValue(const char *name, S32Var_t *s32);
        void addNewConfigValue(const char *name, S64Var_t *s64);

    private:
        // Configuration Variable List for direct access
        ConfigValue_t *config = NULL;

        // Configuration Access via BLE
        S32Var_t BLE_PWD;

        // WLAN Configuration Variables Storage
        S32Var_t WLAN_SSID;
        S64Var_t WLAN_PWD;

        // MQTT Configuration
        S32Var_t MQTT_BROKER;
        UI32Var_t MQTT_PORT;
        S32Var_t MQTT_USER;
        S32Var_t MQTT_PWD;
        
        // Status
        WIFISTATE wifistate = WIFISTATE::UNINITIALIZED;
        char bleSN[2*6+1];
        char devName[16+3*6];
        char mqttTopicRoot[32+16+3*6];
        char ipadd[3*4+3+2];

        int lastStoreRequest = -1;
        unsigned long lastMillis = 0;
        unsigned long txMillis = 0;

        static void freeConfig(ConfigValue_t *c);
        void freeConfig();

        static uint32_t calcConfigCRC(ConfigValue_t *c, void *data);
        static uint32_t calcConfigCRC(ConfigValue_t *c);

        static ConfigValue_t *findValue(ConfigValue_t *l, const char *name);
        ConfigValue_t *findValue(const char *name);
        static void dumpConfig(ConfigValue_t *c);
        void dumpConfig();

        static int min(int a, int b);
        static int min(int a, int b, int c);
        static int min(int a, int b, int c, int d);
        static int max(int a, int b);
        static int max(int a, int b, int c);
        static int max(int a, int b, int c, int d);
        static void incBLEUUIDBy(BLEUUID *b, uint16_t ofs);
        static void incBLEUUIDByOne(BLEUUID *b);
        static char toUpper(char c);
        static void toUpper(char *tgt, const char *src);
        static int decodeStringToBool(const char *s, uint32_t *val);
        static int hexFromChar(char c);
        static int decodeHexByte(const char *c, uint8_t *b);
        static int decodeStringToColor(const char *s, uint32_t *val);
        static int decodeStringToUInt32(const char *s, uint32_t *val);
        static int decodeStringToInt32(const char *s, int32_t *val);

        void defaults();

        static ConfigValue_t *newConfigValue(const char *name, uint8_t flags, VARTYPE vartype, uint16_t slen, void *data);
        ConfigValue_t *addNewConfigValue(const char *name, uint8_t flags, VARTYPE vartype, uint16_t slen, void *data);

        void addConfigValue(ConfigValue_t *c);
        void addNewConfigValue(const char *name, uint8_t flags, UI32Var_t *ui32);
        void addNewConfigValue(const char *name, uint8_t flags, I32Var_t *i32);
        void addNewConfigValue(const char *name, uint8_t flags, F32Var_t *f32);
        void addNewConfigValue(const char *name, uint8_t flags, S16Var_t *s16);
        void addNewConfigValue(const char *name, uint8_t flags, S32Var_t *s32);
        void addNewConfigValue(const char *name, uint8_t flags, S64Var_t * s64);

        int writeConfigToFile(File *f);
        int readConfigFromFile(File *f, bool nochange);

        void loadConfigFromFlash();
        void storeConfigToFlash();

        void createBLEVar(BLEService *svc, const char *name, BLEUUID UUID, bool writeable, bool withDesc = false);
        BLEService *createDevInfoService(BLEServer *pServer);
        BLEService *createConfigService(BLEServer *pServer);
        BLEService *createParameterService(BLEServer *pServer);
        void startBLE();
        void mqttSubscribe();
        void mqttPublish(ConfigValue_t *cfg);
        void mqttPublish();

    public : 
        void onBLEWrite(BLECharacteristic *pCharacteristic);
        void onBLERead(BLECharacteristic *pCharacteristic);

    public : 
        MultiServiceIO();
        void setup();
        void loop();
        void updateMQTT();
        void storeRequired();
};

#endif // __MULTISERVICE_IO_H__
// EOF
