#include "MultiServiceIO.h"

// Global Static Variables
WiFiClient wificlient;
PubSubClient psclient(wificlient);
MultiServiceIO *global_mio = NULL;

// Some Utility Functions
int MultiServiceIO::min(int a, int b) { return (a < b) ? a : b; }
int MultiServiceIO::min(int a, int b, int c) { int bc = min(b, c); return (a < bc) ? a : bc; }
int MultiServiceIO::min(int a, int b, int c, int d) { return min(min(a,b),min(b,c)); }
int MultiServiceIO::max(int a, int b) { return (a < b) ? b : a; }
int MultiServiceIO::max(int a, int b, int c) { int bc = max(b, c); return (a < bc) ? bc : a; }
int MultiServiceIO::max(int a, int b, int c, int d) { return max(max(a, b), max(b, c)); }


void MultiServiceIO::incBLEUUIDByOne(BLEUUID *b)
{
    if (!b) 
        return;
    esp_bt_uuid_t *n = b->getNative();
    if (!n || (n->len != ESP_UUID_LEN_128)) 
        return;
    if (n->uuid.uuid128[12] == 255) {
        n->uuid.uuid128[12] = 0;
        if (n->uuid.uuid128[13] == 255) {
            n->uuid.uuid128[13] = 0;
            if (n->uuid.uuid128[14] == 255) {
                n->uuid.uuid128[14] = 0;
                n->uuid.uuid128[15]++;
            } else
                n->uuid.uuid128[14]++;
        } else 
            n->uuid.uuid128[13]++;
    } else
        n->uuid.uuid128[12]++;
}

void MultiServiceIO::incBLEUUIDBy(BLEUUID *b, uint16_t ofs)
{
    if (!b)
        return;
    esp_bt_uuid_t *n = b->getNative();
    if (!n || (n->len != ESP_UUID_LEN_128))
        return;
    uint32_t base =
        ((uint32_t)n->uuid.uuid128[12]) |
        (((uint32_t)n->uuid.uuid128[13]) << 8) |
        (((uint32_t)n->uuid.uuid128[14]) << 16) |
        (((uint32_t)n->uuid.uuid128[15]) << 24);
    base += ofs;
    n->uuid.uuid128[12] = base & 0x000000FF;
    n->uuid.uuid128[13] = (base >> 8) & 0x000000FF;
    n->uuid.uuid128[14] = (base >> 16) & 0x000000FF;
    n->uuid.uuid128[15] = (base >> 24) & 0x000000FF;
}

// Allocate Memory for a new configuration variable
ConfigValue_t *MultiServiceIO::newConfigValue(const char *name, uint8_t flags, VARTYPE vartype, uint16_t slen, void *data)
{
    if (!name || !data)
        return NULL;
    if ((strlen(name) < 1) || (strlen(name) > MAXNAMEMELN))
        return NULL;
    ConfigValue_t *c = (ConfigValue_t *)malloc(sizeof(ConfigValue_t));
    if (!c) return NULL;
    memset((void *)c, 0, sizeof(ConfigValue_t)); // Zero it
    switch (vartype) {
        case VARTYPE::ui32:
        case VARTYPE::i32:
        case VARTYPE::f32:
            c->storelen = 4;
            break;
        case VARTYPE::str:
            if ((slen != 16) && (slen != 32) && (slen != 64)) {
                // Illegal string length
                free(c);
                return NULL;
            }                
            c->storelen = slen+1;
            break;
        default: 
            // Illegal variable type
            free(c);
            return NULL;
            break;
    }
    c->vartype = vartype;
    c->flags = flags;
    strcpy(c->name, name);
    c->val = (GENERIC_t *)data;
    return c;
}

ConfigValue_t *MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, VARTYPE vartype, uint16_t slen, void *data) {
    ConfigValue_t *cfg = newConfigValue(name, flags, vartype, slen, data);
    if (!cfg)
    {
        Serial.print("Out of memory or illegal variable declared. Name = ");
        if (name) {
            Serial.print("[");
            Serial.print(name);
            Serial.println("]");
        } else 
            Serial.println("NULL");
        return NULL;
    }
    addConfigValue(cfg);
    return cfg;
}

void MultiServiceIO::addNewConfigValue(const char *name, UI32Var_t *ui32, bool asColor)
{
    addNewConfigValue(name, (asColor)?FLAG_COLOR:0, VARTYPE::ui32, 0, (void*) ui32);
}

void MultiServiceIO::addNewConfigValue(const char *name, I32Var_t *i32)
{
    addNewConfigValue(name, 0, VARTYPE::i32, 0, (void *)i32);
}

void MultiServiceIO::addNewConfigValue(const char *name, F32Var_t *f32)
{
    addNewConfigValue(name, 0, VARTYPE::f32, 0, (void *)f32);
}

void MultiServiceIO::addNewConfigValue(const char *name, S16Var_t *s16)
{
    addNewConfigValue(name, 0, VARTYPE::str, 16, (void *)s16);
}

void MultiServiceIO::addNewConfigValue(const char *name, S32Var_t *s32)
{
    addNewConfigValue(name, 0, VARTYPE::str, 32, (void *)s32);
}

void MultiServiceIO::addNewConfigValue(const char *name, S64Var_t *s64)
{
    addNewConfigValue(name, 0, VARTYPE::str, 64, (void *)s64);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, UI32Var_t *ui32)
{
    addNewConfigValue(name, flags, VARTYPE::ui32, 0, (void *)ui32);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, I32Var_t *i32)
{
    addNewConfigValue(name, flags, VARTYPE::i32, 0, (void *)i32);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, F32Var_t *f32)
{
    addNewConfigValue(name, flags, VARTYPE::f32, 0, (void *)f32);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, S16Var_t *s16)
{
    addNewConfigValue(name, flags, VARTYPE::str, 16, (void *)s16);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, S32Var_t *s32)
{
    addNewConfigValue(name, flags, VARTYPE::str, 32, (void *)s32);
}

void MultiServiceIO::addNewConfigValue(const char *name, uint8_t flags, S64Var_t *s64)
{
    addNewConfigValue(name, flags, VARTYPE::str, 64, (void *)s64);
}

uint32_t MultiServiceIO::calcConfigCRC(ConfigValue_t *c, void *data)
{
    if (!c)
        return 0;
    uint32_t cksum = 0;
    uint8_t *b=NULL;

    cksum += c->storelen;
    b=(uint8_t*)&c->name[0];
    for (int i=0;i<MAXNAMEMELN+1;i++)
        cksum += b[i];
    cksum += c->vartype;
    b = (uint8_t*)data;
    for (int i = 0; i < c->storelen; i++)
        cksum += b[i];
    return cksum;
}

uint32_t MultiServiceIO::calcConfigCRC(ConfigValue_t *c)
{
    if (!c) return 0;
    switch (c->vartype) {
    case VARTYPE::ui32:
        return calcConfigCRC(c, &c->val->ui32.val);
    case VARTYPE::i32:
        return calcConfigCRC(c, &c->val->i32.val);
    case VARTYPE::f32:
        return calcConfigCRC(c, &c->val->f32.val);
    case VARTYPE::str:
        return calcConfigCRC(c, &c->val->str.str[0]);
    default:
        return 0;
    }
}

ConfigValue_t *MultiServiceIO::findValue(const char *name) 
{
    if (!config)
        return NULL;
    if (!name)
        return NULL;
    if (!(*name))
        return NULL;
    ConfigValue_t *l = config;
    while (l)
    {
        if (strcmp(l->name, name) == 0)
            return l;
        l = l->next;
    }
    return NULL;
}

void MultiServiceIO::freeConfig(ConfigValue_t *c)
{
    if (!c) return;
    while (c)
    {
        ConfigValue_t *n = c->next;
        free(c);
        c = n;
    }
}

void MultiServiceIO::freeConfig() {
    if (!config) return;
    freeConfig(config);
    config = NULL;
}

void MultiServiceIO::addConfigValue(ConfigValue_t *c)
{
    if (!c)
        return;
    c->next = NULL; // Safety
    if (!config)
    {
        config = c;
        return;
    }
    ConfigValue_t *n = config;
    while (n->next)
        n = n->next;
    n->next = c;
}

void MultiServiceIO::defaults() 
{
    // Set Defaults
    STRSet(BLE_PWD, BLE_DEFAULT_PW);

    STRSet(WLAN_SSID, "");
    STRSet(WLAN_PWD, "");

    STRSet(MQTT_BROKER, "");
    X32Set(MQTT_PORT, 1, 1883, 65535);
    STRSet(MQTT_USER, "");
    STRSet(MQTT_PWD, "");

    // Flush all Configuration variables if there are any
    freeConfig();

    addNewConfigValue("BLEPWD", FLAG_SYSTEM | FLAG_PASSWORD, &BLE_PWD);
    addNewConfigValue("WLANSSID", FLAG_SYSTEM, &WLAN_SSID);
    addNewConfigValue("WLANPWD", FLAG_SYSTEM | FLAG_PASSWORD, &WLAN_PWD);
    addNewConfigValue("MQTTBROKER", FLAG_SYSTEM, &MQTT_BROKER);
    addNewConfigValue("MQTTPORT", FLAG_SYSTEM, &MQTT_PORT);
    addNewConfigValue("MQTTUSER", FLAG_SYSTEM, &MQTT_USER);
    addNewConfigValue("MQTTPWD", FLAG_SYSTEM | FLAG_PASSWORD, &MQTT_PWD);
}

BLEService* MultiServiceIO::createDevInfoService(BLEServer *pServer) {
    BLEService *pInfoService = pServer->createService(BLEUUID((uint16_t)0x180A), (uint32_t) 13);
    BLECharacteristic *pChar = NULL;
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A29), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(BLE_MFG_NAME);
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A24), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(BLE_MODEL_NUMBER);
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A25), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(bleSN);
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A26), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(BLE_FW_REV);
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A27), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(BLE_HW_REV);
    pChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A28), BLECharacteristic::PROPERTY_READ);
    pChar->setValue(BLE_SW_REV);
    return pInfoService;
}

void bleCharacteristicCallback::onWrite(BLECharacteristic *pCharacteristic)
{
    if (!m) return;
    m->onBLEWrite(pCharacteristic);
}

void bleCharacteristicCallback::onRead(BLECharacteristic *pCharacteristic)
{
    if (!m) return;
    m->onBLERead(pCharacteristic);
}

void bleServerCallbacks::onConnect(BLEServer *pServer)
{
    if (!m) return;
    m->BLEConnect();
};

void bleServerCallbacks::onDisconnect(BLEServer *pServer)
{
    if (!m) return;
    m->BLEDisconnect();
}

void MultiServiceIO::onBLEWrite(BLECharacteristic *pCharacteristic)
{
    if (pCharacteristic == txChar) {
        // Special Characteristic
        uint8_t *data = pCharacteristic->getData();
        uint16_t p1, plen;
        memcpy(&p1, &data[0], 2);
        memcpy(&plen, &data[2], 2);
        Serial.printf("Requested %d Packets starting at %d\n", plen, p1);
        if (plen == 0) return;
        if (packetsTX == 0) {
            // First run --> build initial Packet
            buildTXPackage(0);
        }
        if (p1 >= packetsTX)
            return;
        if ((p1 + plen-1) >= packetsTX)
            plen = packetsTX-1 - p1 + 1;
        packstart = p1;
        packstop = p1 + plen - 1;
        notifyTX = true;
        Serial.println("Starting TX");
        return;
    }

    ConfigValue_t *c = config;
    const char *s = NULL;
    while (c) {
        if (c->bleChar == pCharacteristic) {
            // Match
            // Special Handling for the BLE Password
            if (ISSYSTEM(c) && !BLEUnlocked) {
                // No Access to System Variables when locked!
                if (strcmp(c->name, "BLEPWD") == 0) {
                    // This is an unlock Request
                    s = pCharacteristic->getValue().c_str();
                    if (strlen(BLE_PWD.str) < 1)
                        return;
                    if (!s || (strlen(s) < 1))
                        return;
                    if (strcmp(BLE_PWD.str, s) == 0)
                    {
                        // Match
                        BLEUnlock();
                    }
                    // Flush the Password
                    char dummy[9];strcpy(dummy, "********");
                    pCharacteristic->setValue(dummy);
                }
                return; 
            }
            // Non-System Variable or Unlocked already
            union {
                uint32_t ui32;
                int32_t i32;
                float f32;
            } val;
            switch (c->vartype)
            {
            case VARTYPE::ui32:
                memcpy(&val, pCharacteristic->getData(), 4);
                if (val.ui32 < c->val->ui32.min) val.ui32 = c->val->ui32.min;
                if (val.ui32 > c->val->ui32.max) val.ui32 = c->val->ui32.max;
                if (val.ui32 != c->val->ui32.val) {
                    SETBLE(c->val->ui32);
                    c->val->ui32.val = val.ui32;
                    storeRequired();
                }
                if (ISPASSWORD(c)) {
                    uint32_t dummy = 0;
                    pCharacteristic->setValue((uint8_t *)&dummy, 4);
                }
                break;
            case VARTYPE::i32:
                memcpy(&val, pCharacteristic->getData(), 4);
                if (val.i32 < c->val->i32.min) val.i32 = c->val->i32.min;
                if (val.i32 > c->val->i32.max) val.i32 = c->val->i32.max;
                if (val.i32 != c->val->i32.val) {
                    SETBLE(c->val->i32);
                    c->val->i32.val = val.i32;
                    storeRequired();
                }
                if (ISPASSWORD(c)) {
                    uint32_t dummy = 0;
                    pCharacteristic->setValue((uint8_t *)&dummy, 4);
                }
                break;
            case VARTYPE::f32:
                memcpy(&val, pCharacteristic->getData(), 4);
                if (val.f32 < c->val->f32.min)
                    val.f32 = c->val->f32.min;
                if (val.f32 > c->val->f32.max)
                    val.f32 = c->val->f32.max;
                if (val.f32 != c->val->f32.val)
                {
                    SETBLE(c->val->f32);
                    c->val->f32.val = val.f32;
                    storeRequired();
                }
                if (ISPASSWORD(c)) {
                    float dummy = 0;
                    pCharacteristic->setValue((uint8_t *)&dummy, 4);
                }
                break;
            case VARTYPE::str:
                s = pCharacteristic->getValue().c_str();
                if (!s) return; // Bad Parameter
                if (strlen(s) > c->storelen-1)
                    return; // Too long
                if (strcmp(c->val->str.str, s)) {
                    // Is different
                    SETBLE(c->val->str);
                    memset(&c->val->str.str, 0, c->storelen);
                    memcpy(c->val->str.str, s, strlen(s));
                    storeRequired();
                }
                if (ISPASSWORD(c)) {
                    char dummy[9]; strcpy(dummy,"********");
                    pCharacteristic->setValue(dummy);
                }
                break;
            default:
                break;
            }
            return;
        }
        c = c->next;
    }
}

void MultiServiceIO::onBLERead(BLECharacteristic *pCharacteristic)
{
    // Nothing to do on read
}

void MultiServiceIO::BLEConnect()
{
    if (BLEConnected)
        return;
    BLEConnected = true;
    Serial.println("BLE is connected");
}

void MultiServiceIO::BLEDisconnect()
{
    if (!BLEConnected)
        return;
    Serial.println("BLE Access is disconnected");
    BLEConnected = false;
    if (BLEUnlocked)
    {
        Serial.println("BLE Access is locked");
        BLEUnlocked = false;
        ConfigValue_t *cfg = config;
        while (cfg)
        {
            if (ISSYSTEM(cfg))
            {
                // Hide content
                if (cfg->bleChar)
                {
                    cfg->bleChar->setValue(NULL, 0);
                }
            }
            cfg = cfg->next;
        }
    }
}

void MultiServiceIO::BLEUnlock()
{
    if (BLEUnlocked)
        return;
    Serial.println("BLE Access is unlocked!");
    BLEUnlocked = true;
    ConfigValue_t *cfg = config;
    while (cfg)
    {
        if (ISSYSTEM(cfg))
        {
            // Show content
            if (cfg->bleChar)
            {
                switch (cfg->vartype)
                {
                case VARTYPE::ui32:
                    if (ISPASSWORD(cfg))
                    {
                        uint32_t dummy = 0;
                        cfg->bleChar->setValue((uint8_t *)&dummy, 4);
                    }
                    else
                        cfg->bleChar->setValue((uint8_t *)&cfg->val->ui32.val, 4);
                    break;
                case VARTYPE::i32:
                    if (ISPASSWORD(cfg))
                    {
                        uint32_t dummy = 0;
                        cfg->bleChar->setValue((uint8_t *)&dummy, 4);
                    }
                    else
                        cfg->bleChar->setValue((uint8_t *)&cfg->val->i32.val, 4);
                    break;
                case VARTYPE::f32:
                    if (ISPASSWORD(cfg))
                    {
                        float dummy = 0;
                        cfg->bleChar->setValue((uint8_t *)&dummy, 4);
                    }
                    else
                        cfg->bleChar->setValue((uint8_t *)&cfg->val->f32.val, 4);
                    break;
                case VARTYPE::str:
                    if (ISPASSWORD(cfg))
                    {
                        char dummy[9];
                        strcpy(dummy, "********");
                        cfg->bleChar->setValue(dummy);
                    }
                    else
                        cfg->bleChar->setValue(cfg->val->str.str);
                    break;
                default:
                    break;
                }
            }
        }
        cfg = cfg->next;
    }
}

void MultiServiceIO::pushU8(uint32_t *idx, uint32_t first, uint32_t last, uint8_t data)
{
    if ((*idx) < first)
    {
        (*idx)++;
        return;
    }
    if ((*idx) > last)
    {
        (*idx)++;
        return;
    }
    notify.data[(*idx) - first] = data;
    (*idx)++;
}

void MultiServiceIO::push(uint32_t *idx, uint32_t first, uint32_t last, uint8_t *data, uint32_t len)
{
    if ((*idx) + len - 1 < first)
    {
        (*idx) += len;
        return;
    }
    if ((*idx) > last)
    {
        (*idx) += len;
        return;
    }
    while (len)
    {
        pushU8(idx, first, last, *data);
        data++;
        len--;
    }
}

bool MultiServiceIO::buildTXPackage(uint16_t packet)
{
    if (packet == 0)
    {
        // First one --> calc total packet number and so on
        uint32_t txlen = 0;
        ConfigValue_t *cfg = config;
        while (cfg) {
            if (!ISSYSTEM(cfg)) {
                txlen += 1; // Name length indicator
                txlen += strlen(cfg->name);
                txlen += 1; // Var Type

                switch (cfg->vartype) {
                case VARTYPE::ui32:
                case VARTYPE::i32:
                case VARTYPE::f32:
                    txlen += 4 * 3; // min, max, nom;
                    break;
                case VARTYPE::str:
                    txlen += 1; // Storelen
                    txlen += cfg->storelen-1;
                    break;
                default: // Ignore;
                    break;
                }
            }
            cfg=cfg->next;
        }
        packetsTX = txlen/18; // 18 bytes per package
        if ((packetsTX * 18) < txlen) packetsTX++;

        packetsTX++; // +initial Packet

        memset(&notify.data[0],0,18);
        notify.packetID = 0;
        memcpy(&notify.data[0], &packetsTX, 2);
        memcpy(&notify.data[2], &txlen, 4);

        Serial.printf("Assembled initial Notify Packet (%d packets)\n", packetsTX);
        return true;
    }

    if (packet >= packetsTX) {
        Serial.println("Access beyond last packet");
        return false; // Beyond last packet
    }

    // Assemble intermediate packet
    notify.packetID = packet;
    memset(&notify.data[0], 0, 18);
    uint32_t idx = 0;
    uint32_t first = (packet-1)*18;
    uint32_t last = (packet-1) * 18 + 17;
    ConfigValue_t *cfg = config;
    while (cfg && (idx <= last)) {
        if (!ISSYSTEM(cfg)) {
            uint8_t namelen = strlen(cfg->name);
            pushU8(&idx, first, last, namelen); 
            push(&idx, first, last, (uint8_t *)&cfg->name[0], namelen);
            pushU8(&idx, first, last, cfg->vartype);
            switch (cfg->vartype) {
            case VARTYPE::ui32:
                push(&idx, first, last, (uint8_t *)&cfg->val->ui32.min, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->ui32.max, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->ui32.val, 4);
                break;
            case VARTYPE::i32:
                push(&idx, first, last, (uint8_t *)&cfg->val->i32.min, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->i32.max, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->i32.val, 4);
                break;
            case VARTYPE::f32:
                push(&idx, first, last, (uint8_t *)&cfg->val->f32.min, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->f32.max, 4);
                push(&idx, first, last, (uint8_t *)&cfg->val->f32.val, 4);
                break;
            case VARTYPE::str:
                pushU8(&idx, first, last, cfg->storelen-1);
                push(&idx, first, last, (uint8_t *)&cfg->val->str.str[0], cfg->storelen-1);
                break;
            default:
                break;
            }
        }
        cfg = cfg->next;
    }
    return true;
}

void MultiServiceIO::createBLEVar(BLEService *svc, const char *name, BLEUUID UUID, bool writeable, bool withDesc)
{
    ConfigValue_t *cfg = findValue(name);
    if (!cfg) return;
    
    BLECharacteristic *pChar = svc->createCharacteristic(UUID, 
        BLECharacteristic::PROPERTY_READ | ((writeable)?BLECharacteristic::PROPERTY_WRITE:0));
    if (!pChar) return;

    cfg->bleChar = pChar;
    pChar->setCallbacks(&blecbk);

    // Value
    switch (cfg->vartype)
    {
    case VARTYPE::ui32:
        if (ISSYSTEM(cfg)) {
            // System Variable, only accessible if unlocked
            if (BLEUnlocked) {
                if (ISPASSWORD(cfg)) {
                    // Never transmit
                    uint32_t dummy = 0;
                    pChar->setValue((uint8_t *)&dummy, 4);
                } else {
                    // Transmit
                    pChar->setValue((uint8_t *)&cfg->val->ui32.val, 4);
                }
            } else {
                pChar->setValue(NULL,0); // Nothing
            }
        } else {
            if (ISPASSWORD(cfg)) { // Never transmit
                uint32_t dummy = 0;
                pChar->setValue((uint8_t *)&dummy, 4);
            } else
                pChar->setValue((uint8_t *)&cfg->val->ui32.val, 4);
        }
        break;
    case VARTYPE::i32:
        if (ISSYSTEM(cfg)) {
            // System Variable, only accessible if unlocked
            if (BLEUnlocked) {
                if (ISPASSWORD(cfg)) {
                    // Never transmit
                    uint32_t dummy = 0;
                    pChar->setValue((uint8_t *)&dummy, 4);
                } else {
                    // Transmit
                    pChar->setValue((uint8_t *)&cfg->val->i32.val, 4);
                }
            } else {
                pChar->setValue(NULL, 0); // Nothing
            }
        } else {
            if (ISPASSWORD(cfg)) {
                // Never transmit
                uint32_t dummy = 0;
                pChar->setValue((uint8_t *)&dummy, 4);
            } else {
                // Transmit
                pChar->setValue((uint8_t *)&cfg->val->i32.val, 4);
            }
        }
        break;
    case VARTYPE::f32:
        if (ISSYSTEM(cfg)) {
            // System Variable, only accessible if unlocked
            if (BLEUnlocked) {
                if (ISPASSWORD(cfg)) {
                    // Never transmit
                    uint32_t dummy = 0;
                    pChar->setValue((uint8_t *)&dummy, 4);
                } else {
                    // Transmit
                    pChar->setValue((uint8_t *)&cfg->val->f32.val, 4);
                }
            } else {
                pChar->setValue(NULL, 0); // Nothing
            }
        } else {
            if (ISPASSWORD(cfg)) {
                // Never transmit
                uint32_t dummy = 0;
                pChar->setValue((uint8_t *)&dummy, 4);
            } else {
                // Transmit
                pChar->setValue((uint8_t *)&cfg->val->f32.val, 4);
            }
        }
        break;
    case VARTYPE::str:
        if (ISSYSTEM(cfg)) {
            // System Variable, only accessible if unlocked
            if (BLEUnlocked) {
                if (ISPASSWORD(cfg)) {
                    // Never transmit
                    char dummy[9];
                    strcpy(dummy, "********");
                    pChar->setValue((uint8_t *)&dummy[0], strlen(dummy));
                } else {
                    // Transmit
                    pChar->setValue((uint8_t *)&cfg->val->str.str[0], strlen(cfg->val->str.str));
                }
            } else {
                pChar->setValue(NULL, 0); // Nothing
            }
        } else {
            if (ISPASSWORD(cfg)) {
                // Never Transmit
                char dummy[9];
                strcpy(dummy, "********");
                pChar->setValue((uint8_t *)&dummy[0], strlen(dummy));
            } else {
                // Transmit
                pChar->setValue((uint8_t *)&cfg->val->str.str[0], strlen(cfg->val->str.str));
            }
        }
        break;
    default:
        return; // Illegal Var Type
    }

    if (!withDesc)
        return;

    return;

    // Descriptors
    BLEDescriptor *d0 = NULL;
    // BLE2902 *d1 = NULL;
    BLE2904 *d2 = NULL;
    BLEDescriptor *d3 = NULL;

    // Name Descriptor
    d0 = new BLEDescriptor(BLEUUID((uint16_t) 0x2901), strlen(name));
    if (!d0) return;
    d0->setValue((uint8_t*)&name[0], strlen(name));
    pChar->addDescriptor(d0);

    // Format
    d2 = new BLE2904();
    if (!d2) return;
    switch (cfg->vartype)
    {
    case VARTYPE::ui32:
        d2->setFormat(BLE2904::FORMAT_UINT32);
        break;
    case VARTYPE::i32:
        d2->setFormat(BLE2904::FORMAT_SINT32);
        break;
    case VARTYPE::f32:
        d2->setFormat(BLE2904::FORMAT_FLOAT32);
        break;
    case VARTYPE::str:
        d2->setFormat(BLE2904::FORMAT_UTF8);
        break;
    default:
        return; // Illegal Var Type!
    }
    pChar->addDescriptor(d2);
    
    // Add Limit Descriptor for numeric values
    union {
        struct { uint32_t min; uint32_t max; } ui32;
        struct { int32_t min; int32_t max; } i32;
        struct { float min; float max; } f32;
    } limits;
    switch (cfg->vartype)
    {
    case VARTYPE::ui32:
        limits.ui32.min = cfg->val->ui32.min; 
        limits.ui32.max = cfg->val->ui32.max;
        d3 = new BLEDescriptor(BLEUUID((uint16_t)0x2906), 8);
        if (!d3) return;
        d3->setValue((uint8_t*)&limits,8);
        pChar->addDescriptor(d3);
        break;
    case VARTYPE::i32:
        limits.i32.min = cfg->val->i32.min;
        limits.i32.max = cfg->val->i32.max;
        d3 = new BLEDescriptor(BLEUUID((uint16_t)0x2906), 8);
        if (!d3) return;
        d3->setValue((uint8_t *)&limits, 8);
        pChar->addDescriptor(d3);
        break;
    case VARTYPE::f32:
        limits.f32.min = cfg->val->f32.min;
        limits.f32.max = cfg->val->f32.max;
        d3 = new BLEDescriptor(BLEUUID((uint16_t)0x2906), 8);
        if (!d3) return;
        d3->setValue((uint8_t *)&limits, 8);
        pChar->addDescriptor(d3);
        break;
    default:
        return; // Illegal Var Type!
    }
}

BLEService *MultiServiceIO::createConfigService(BLEServer *pServer)
{
    // Count parameters
    int prm = 0;
    ConfigValue_t *p = config;
    while (p)
    {
        if (ISSYSTEM(p)) prm++;
        p = p->next;
    }
    Serial.print("Found "); Serial.print(prm); Serial.println(" System Parameters to share via BLE");
    BLEUUID uuid = BLEUUID(BLE_BASE_UUID);
    BLEService *pCfgService = pServer->createService(uuid, (uint32_t)2 * prm + 1);
    incBLEUUIDByOne(&uuid);
    p = config;
    while (p)
    {
        if (ISSYSTEM(p))
        {
            createBLEVar(pCfgService, p->name, uuid, true, false);
            incBLEUUIDByOne(&uuid);
        }
        p = p->next;
    }
    return pCfgService;
}

BLEService *MultiServiceIO::createParameterService(BLEServer *pServer) 
{
    // Count parameters
    int prm = 0;
    ConfigValue_t *p = config;
    while (p)
    {
        if (!ISSYSTEM(p)) prm++;
        p = p->next;
    }
    Serial.print("Found ");Serial.print(prm);Serial.println(" Parameters to share via BLE");
    BLEUUID uuid = BLEUUID(BLE_BASE_UUID); incBLEUUIDBy(&uuid, 0x1000);

    BLEService *pService = pServer->createService(uuid, (uint32_t)4 * prm + 3 + 1);
    incBLEUUIDByOne(&uuid);

    txChar = pService->createCharacteristic(uuid,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    if (!txChar)
        return NULL;
    BLEDescriptor *bd = new BLE2902();
    if (!bd)
        return NULL;
    txChar->addDescriptor(bd);
    uint32_t dummy = 0;
    txChar->setValue((uint8_t*)&dummy, 4);
    txChar->setCallbacks(&blecbk);
    incBLEUUIDByOne(&uuid);

    p = config;
    while (p) {
        if (!ISSYSTEM(p)) {
            createBLEVar(pService, p->name, uuid, true, true);
            incBLEUUIDByOne(&uuid);
        }
        p = p->next;
    }
    return pService;
}

void MultiServiceIO::startBLE()
{
    blecbk.m = this;
    blesrvcbk.m = this;

    BLEDevice::init(BLE_NAME);
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(&blesrvcbk);
    devInfoSvc = createDevInfoService(bleServer);
    cfgSvc = createConfigService(bleServer);
    prmSvc = createParameterService(bleServer);
    devInfoSvc->start();
    cfgSvc->start();
    prmSvc->start();
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x180A)); // Device Info Svc
    pAdvertising->addServiceUUID(BLE_BASE_UUID);
    // Note: Config Service is not being advertized by default, otherwise uncomment the following line
    pAdvertising->addServiceUUID(BLE_BASE_UUID);
    pAdvertising->start();
}

MultiServiceIO::MultiServiceIO() {
    Serial.println("Initializing MultiServiceIO Object");
    defaults();
    Serial.println("Object initialized");
}

int MultiServiceIO::writeConfigToFile(File *f) 
{
    ConfigValue_t *cfg = config;
    while (cfg)
    {
        uint32_t cksum = calcConfigCRC(cfg); // Can never be 0 for a real data set
        if (
            (f->write((uint8_t *)&cksum, 4) != 4) ||
            (f->write((uint8_t *)&cfg->vartype, 1) != 1) ||
            (f->write((uint8_t *)&cfg->name[0], MAXNAMEMELN + 1) != MAXNAMEMELN + 1))
            return 0; // Fail
        switch (cfg->vartype) {
            case VARTYPE::ui32:
                if (f->write((uint8_t *)&cfg->val->ui32.val, 4) != 4) return 0; // Fail
                break;
            case VARTYPE::i32:
                if (f->write((uint8_t *)&cfg->val->i32.val, 4) != 4) return 0; // Fail
                break;
            case VARTYPE::f32:
                if (f->write((uint8_t *)&cfg->val->f32.val, 4) != 4) return 0; // Fail
                break;
            case VARTYPE::str:
                if (f->write((uint8_t*)&cfg->storelen, 2) != 2) return 0; // Fail
                if (f->write((uint8_t *)&cfg->val->str.str[0], cfg->storelen) != cfg->storelen)
                    return 0; // Fail
                break;
            default:
                // Bad variable type --> fail
                return 0;
            }
        cfg = cfg->next;
    }
    // Add a Zero to mark the end of the file
    uint32_t dummy = 0;
    if (f->write((uint8_t *)&dummy, 4) != 4)
        return 0; // Fail
    return 1; // Success
}

void MultiServiceIO::dumpConfig(ConfigValue_t *c) {
    Serial.println("CONFIG DUMP BEGIN");
    while (c) {
        Serial.printf("NAME [%s]\n", c->name);
        Serial.printf("\tStorelen = %d\n",c->storelen);
        Serial.print("\tFlags = ");
        if (ISSYSTEM(c)) Serial.print("SYSTEM ");
        if (ISPASSWORD(c)) Serial.print("PWD");
        Serial.println();
        Serial.print("\tCHANGED = ");
        if (!c->val) {
            Serial.print("[NULL]");
        } else {
            if (ISLOCAL(c->val->any))
                Serial.print("LOCAL ");
            if (ISBLE(c->val->any))
                Serial.print("BLE ");
            if (ISMQTT(c->val->any))
                Serial.print("MQTT ");
        }
        Serial.println();
        switch (c->vartype)
        {
        case VARTYPE::ui32:
            Serial.println("\tType = UINT32");
            break;
        case VARTYPE::i32:
            Serial.println("\tType = INT32");
            break;
        case VARTYPE::f32:
            Serial.println("\tType = FLOAT32");
            break;
        case VARTYPE::str:
            Serial.println("\tType = STRING");
            break;
        default:
            Serial.printf("\tType = UNKNOWN (%d)\n", c->vartype);
            break;
        }
        if (!c->val) {
            Serial.println("\tVALUE = [NULL]");
        } else {
            switch (c->vartype)
            {
            case VARTYPE::ui32:
                Serial.printf("\tValue = %u\n", c->val->ui32.val);
                Serial.printf("\tMin = %u\n", c->val->ui32.min);
                Serial.printf("\tmax = %u\n", c->val->ui32.max);
                break;
            case VARTYPE::i32:
                Serial.printf("\tValue = %d\n", c->val->i32.val);
                Serial.printf("\tMin = %d\n", c->val->i32.min);
                Serial.printf("\tmax = %d\n", c->val->i32.max);
                break;
            case VARTYPE::f32:
                Serial.printf("\tValue = %f\n", c->val->f32.val);
                Serial.printf("\tMin = %f\n", c->val->f32.min);
                Serial.printf("\tmax = %f\n", c->val->f32.max);
                break;
            case VARTYPE::str:
                Serial.printf("\tValue = [%s]\n", c->val->str.str);
                break;
            default:
                break;
            }
        }
        c = c->next;
    }
    Serial.println("CONFIG DUMP END");
}

void MultiServiceIO::dumpConfig() {
    dumpConfig(config);
}

int MultiServiceIO::readConfigFromFile(File *f, bool nochange)
{
    while (1) {
        // Read Checksum
        uint32_t cksum;
        if (f->readBytes((char*)&cksum,4) != 4) {
            Serial.println("Read Error");
            return 0; // Fail
        }
        if (cksum == 0)
        {
            // Last Block
            return 1; // Success
        }
        // Read Block header
        uint16_t storelen;
        VARTYPE vartype;
        char name[MAXNAMEMELN+2];
        name[MAXNAMEMELN+1] = 0; // Ensure that strlen does not fail
        if (
            (f->readBytes((char *)&vartype, 1) != 1) ||
            (f->readBytes((char *)&name[0], MAXNAMEMELN+1) != MAXNAMEMELN+1)) {
            Serial.println("Read Error");
            return 0; // Fail
        }
        if (strlen(name) > MAXNAMEMELN) {
            Serial.println("Illegal Name Length found in file");
            return 0;
        }
        switch (vartype) {
        case VARTYPE::ui32:
        case VARTYPE::i32:
        case VARTYPE::f32:
            storelen = 4;
            break;
        case VARTYPE::str:
            if (f->readBytes((char*)&storelen, 2) != 2) {
                Serial.println("Read Error");
                return 0; // Fail
            }
            if ((storelen != 16+1) && (storelen != 32+1) && (storelen != 64+1)) {
                Serial.println("Illegan Store Length in File");
                return 0; // Fail
            }
            break;
        default:
            Serial.println("Illegal VarType found");
            return 0;
        }
        // Search variable
        ConfigValue_t *c = findValue(name);
        if (!c || (c->storelen != storelen) || (c->vartype != vartype)) {
            // No fit --> just ignore and issure warning
            if (c) {
                Serial.print("Found Unmatching Variable ");Serial.println(c->name);
            } else {
                Serial.print("Found Unknown Variable ");Serial.println(name);
            }
            // step over it anyway
            uint8_t dummy = 0;
            for (int i=0;i<storelen;i++) 
                if (f->readBytes((char*)&dummy,1) != 1)  {
                    Serial.println("Read Error");
                    return 0;
                }
        } else {
            // is there --> and matches
            uint8_t *data  = (uint8_t*)malloc(storelen+1);
            if (!data) {
                Serial.println("Out of Memory");
                return 0; // Fail
            }
            X32_t *X = (X32_t *)data;
            data[storelen] = 0; // Ensure that strlen does not fail
            if (f->readBytes((char *)data, storelen) != storelen)
            {
                free(data);
                Serial.println("Read Error");
                return 0; // Fail
            }
            uint32_t rdsum = calcConfigCRC(c, (void*)data);
            if (rdsum != cksum) {
                free(data);
                Serial.println("Checksum Error");
                return 0;
            }
            // Store it
            switch (vartype)
            {
            case VARTYPE::ui32:
                if (X->ui32 < c->val->ui32.min) X->ui32 = c->val->ui32.min;
                if (X->ui32 > c->val->ui32.max) X->ui32 = c->val->ui32.max;
                if (!nochange) c->val->ui32.val = X->ui32;
                break;
            case VARTYPE::i32:
                if (X->i32 < c->val->i32.min) X->i32 = c->val->i32.min;
                if (X->i32 > c->val->i32.max) X->i32 = c->val->i32.max;
                if (!nochange) c->val->i32.val = X->i32;
                break;
            case VARTYPE::f32:
                if (X->f32 < c->val->f32.min) X->f32 = c->val->f32.min;
                if (X->f32 > c->val->f32.max) X->f32 = c->val->f32.max;
                if (!nochange) c->val->f32.val = X->f32;
                break;
            case VARTYPE::str:
                if (strlen((char *)&data[0]) >= storelen-1) {
                    // String too long
                    Serial.println("String too long Error");
                    return 0;
                }
                if (!nochange) strcpy(c->val->str.str, (char *)&data[0]);
                break;
            default:
                break;
            }
            // Serial.print("Read Variable ");Serial.println(c->name);
            free(data);
        }
    }
}

void MultiServiceIO::loadConfigFromFlash() {
    File f = SPIFFS.open("/config.bin", "r");
    if (!f) {
        Serial.print("Config could not be read from SPIFFS File\n");
        return;
    }
    if (readConfigFromFile(&f, true) == 1) {
        // Success --> do real read;
        f.close();
        f = SPIFFS.open("/config.bin", "r");
        if (!f) {
            // oops, that should not happen
            Serial.println("Very strange 1 error in " __FILE__);
            return;
        }
        if (readConfigFromFile(&f, false) == 0) {
            // oops, that should not happen
            Serial.println("Very strange 2 error in " __FILE__);
            return;
        }
        Serial.print("Config successfully read from SPIFFS File\n");
    } else 
        Serial.print("Failed to read Config File from SPIFFS File\n");
    f.close();
}

void MultiServiceIO::storeConfigToFlash() {
    File f = SPIFFS.open("/config.bin", "w");
    if (!f) {
        Serial.print("Config file could not be written to SPIFFS File\n");
        return;
    }
    if (writeConfigToFile(&f)) {
        Serial.print("Config File successfully written to SPIFFS File\n");
    } else {
        Serial.print("Failed to write Config File to SPIFFS File\n");
    }
    f.close();
}

void MultiServiceIO::setup() {
    // Config
#ifdef OMIT_SPIFFS
    Serial.println("Omitting SPIFFS Defaults tread");
#else
    Serial.println("Trying to read defaults from SPIFFS");
    loadConfigFromFlash();
#endif

    // Dump Config
    dumpConfig();

    // Init Wifi if configured
    Serial.println("Initializing WLAN Service now");
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(false);
    WiFi.enableAP(false);
    wifistate = WIFISTATE::UNINITIALIZED;

    // Extract Serial Number and device name from MAC
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(bleSN, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(devName, "%s_%02X_%02X_%02X_%02X_%02X_%02X", MQTT_DEV_PREFIX, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(mqttTopicRoot, "%s/%s", MQTT_TOPIC_ROOT, devName);

    Serial.println("TopicRoot: " + String(mqttTopicRoot));

    // Start BLE Server
    Serial.println("Starting BLE Service now");
    Serial.print("BLE Name = ");Serial.println(BLE_NAME);
    startBLE();
}


// extern "C" 
void mqtt_callback(const char *topic, unsigned char *payload, unsigned int length) 
{
    if (global_mio) global_mio->mqttCallback(topic, payload, length);
}

char MultiServiceIO::toUpper(char c) 
{
    if ((c >= 'a') && (c <= 'z')) return c - ('a' - 'A');
    return c;
}

void MultiServiceIO::toUpper(char *tgt, const char *src)
{
    if (!tgt || !src) return;
    if (!*src) return;
    while (*src) {
        *tgt = toUpper(*src);
        tgt++;src++;
    }
    *tgt = 0;
}

int MultiServiceIO::decodeStringToBool(const char *s, uint32_t *val) {
    if (!s || !val) return 0;
    if ((strcmp(s, "1") == 0) || (strcmp(s, "ON") == 0) ||
        (strcmp(s, "HIGH") == 0) || (strcmp(s, "HI") == 0) || 
        (strcmp(s, "CLOSE") == 0))
    {
        *val = 1;
        return 1;
    }
    if ((strcmp(s, "0") == 0) || (strcmp(s, "OFF") == 0) ||
        (strcmp(s, "LOW") == 0) || (strcmp(s, "LO") == 0) || 
        (strcmp(s, "OPEN") == 0))
    {
        *val = 0;
        return 1;
    }
    return 0;
}

int MultiServiceIO::hexFromChar(char c) {
    if ((c >= '0') && (c <= '9')) return c - '0';
    if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
    return -1;
}

int MultiServiceIO::decodeHexByte(const char *c, uint8_t *b)
{
    if (!c || !b) return 0;
    int h1 = hexFromChar(*c); c++;
    int h2 = hexFromChar(*c);
    if ((h1 < 0) || (h2 < 0)) return 0;
    *b = (uint8_t) ((h1 << 4) | h2);
    return 1;
}

int MultiServiceIO::decodeStringToColor(const char *s, uint32_t *val)
{
    if (!s || !val) return 0;
    if (strcmp(s, "BLACK") == 0) { *val = 0x000000; return 1; }
    if (strcmp(s, "WHITE") == 0) { *val = 0xFFFFFF; return 1; }
    if (strcmp(s, "RED") == 0) { *val = 0xFF0000; return 1; }
    if (strcmp(s, "GREEN") == 0) { *val = 0x00FF00; return 1; }
    if (strcmp(s, "BLUE") == 0) { *val = 0x0000FF; return 1; }
    if (strcmp(s, "CYAN") == 0) { *val = 0x00FFFF; return 1; }
    if (strcmp(s, "MAGENTA") == 0) { *val = 0xFF00FF; return 1; }
    if (strcmp(s, "YELLOW") == 0) { *val = 0xFFFF00; return 1; }
    if (strcmp(s, "GRAY") == 0) { *val = 0x808080; return 1; }
    if ((s[0] == '#') && (strlen(s) == 7)) {
        // Hex notation
        uint8_t r,g,b;
        if ((decodeHexByte(&s[1],&r)==0) ||
            (decodeHexByte(&s[3],&g)==0) || 
            (decodeHexByte(&s[5],&b)==0))
            return 0;
        *val = RGB(r,g,b);
        return 1;
    }
    const char *q=s;
    int scnt = 0;
    while (*q) { if (*q == ',') scnt++; q++; }
    if (scnt == 2) {
        // Format r,g,b as 8bit integers
        char temp[6] = {0,0,0,0,0,0};
        char *t = &temp[0];
        q = s;
        while ((*q == ' ') || (*q == '\t')) q++;
        while ((*q != ',') && (*q != ' ') && (*q != '\t')) { *t = *q; t++; q++; }
        *t = 0;
        while (*q != ',') q++;q++;
        char *ep;
        int r = strtol(temp, &ep, 0);
        if (ep && *ep) return 0; 
        t = &temp[0];
        while ((*q == ' ') || (*q == '\t')) q++;
        while ((*q != ',') && (*q != ' ') && (*q != '\t')) { *t = *q; t++; q++; }
        *t = 0;
        while (*q != ',') q++;q++;
        int g = strtol(temp, &ep, 0);
        if (ep && *ep) return 0;
        t = &temp[0];
        while ((*q == ' ') || (*q == '\t')) q++;
        while (*q && (*q != ' ') && (*q != '\t')) { *t = *q; t++; q++; }
        *t = 0;
        int b = strtol(temp, &ep, 0);
        if (ep && *ep) return 0;
        while ((*q == ' ' ) ||  (*q == '\t')) q++;
        if (*q) return 0;
        if ((r < 0) || (r > 255)) return 0;
        if ((g < 0) || (g > 255)) return 0;
        if ((b < 0) || (b > 255)) return 0;
        *val = RGB(r,g,b);
        return 1;
    }
    return 0;
}

int MultiServiceIO::decodeStringToUInt32(const char *s, uint32_t *val)
{
    if (!s || !val) return 0;
    // Note: String should be converted to upper case already!
    if (decodeStringToBool(s, val))
        return 1;
    if (decodeStringToColor(s, val))
        return 1;
    // Try integer
    char *ep;
    *val = strtol(s, &ep, 0);
    if (ep && *ep) return 0; // Format error
    return 1;
}

int MultiServiceIO::decodeStringToInt32(const char *s, int32_t *val)
{
    if (!s || !val) return 0;
    // Note: String should be converted to upper case already!
    if (decodeStringToBool(s, (uint32_t*)val))
        return 1;
    // if (decodeStringToColor(s, val))
    //    return 1;
    // Try integer
    char *ep;
    *val = strtol(s, &ep, 0);
    if (ep && *ep) return 0; // Format error
    return 1;
}

void MultiServiceIO::mqttCallback(const char *topic, unsigned char *payload, unsigned int length)
{
    // return;
    char pl[64];
    if (length > 63)
        return; // Ignore
    for (int i = 0; i < length; i++)
        pl[i] = payload[i];
    pl[length] = 0;
    // Convert to upper case
    toUpper(&pl[0], &pl[0]);

    Serial.print("MQTT: ");
    Serial.print(topic);
    Serial.print(" = [");
    Serial.print(pl);
    Serial.println("]");
    // Topic
    // Example lightball/LB_00_11_22_33_44_55/set/bla_blub
    //         --- ROOT ---------------------
    if (strncmp(topic, mqttTopicRoot, strlen(mqttTopicRoot)))
        return;                         // No match, even the first part
    topic += strlen(mqttTopicRoot) + 1; // set/bla_blub
    if (strncmp(topic, "set/", 4))
        return; // No Match with the set part
    topic += 4; // bla_blub
    ConfigValue_t *cfg = config;
    while (cfg && strcmp(cfg->name, topic)) // Skip System Variables via MQTT!
        cfg = cfg->next;
    if (!cfg)
        return; // No Match
    if (ISSYSTEM(cfg)) 
        return; // System Variables can only be accessed via BLE!

    char *ep;
    uint32_t uval = 0;
    int32_t ival = 0;
    float fval = 0.0;
    switch (cfg->vartype)
    {
    case VARTYPE::ui32:
        // Convert
        if (!decodeStringToUInt32(pl, &uval))
            return; // Format error
        if (uval < cfg->val->ui32.min) uval = cfg->val->ui32.min;
        if (uval > cfg->val->ui32.max) uval = cfg->val->ui32.max;
        if (uval != cfg->val->ui32.val) {
            // Serial.printf("Updating %s with %d\n", cfg->name, uval);
            cfg->val->ui32.val = uval;
            SETMQTT(cfg->val->ui32);
            storeRequired();
        }
        return;
    case VARTYPE::i32:
        // Convert
        if (!decodeStringToInt32(pl, &ival))
            return; // Format error
        if (ival < cfg->val->i32.min) ival = cfg->val->i32.min;
        if (ival > cfg->val->i32.max) ival = cfg->val->i32.max;
        if (ival != cfg->val->i32.val) {
            cfg->val->i32.val = ival;
            SETMQTT(cfg->val->i32);
            storeRequired();
        }
        return;
    case VARTYPE::f32:
        // Convert
        fval = strtof(pl, &ep);
        if (ep && *ep)
            return; // Format error
        if (fval < cfg->val->f32.min) fval = cfg->val->f32.min;
        if (fval > cfg->val->f32.max) fval = cfg->val->f32.max;
        if (fval != cfg->val->f32.val) {
            cfg->val->f32.val = fval;
            SETMQTT(cfg->val->f32);
            storeRequired();
        }
        return;
    case VARTYPE::str:
        if (strlen(pl) >= cfg->storelen)
            return; // Too long
        if (strcmp(pl, cfg->val->str.str)) {
            strcpy(cfg->val->str.str, pl);
            SETMQTT(cfg->val->str);
            storeRequired();
        }
        return;
    default:
        return;
    }
}

void MultiServiceIO::mqttSubscribe()
{
    char setTopic[128];
    sprintf(setTopic, "%s/set/#", mqttTopicRoot);
    psclient.subscribe(setTopic);
}

void MultiServiceIO::mqttPublish(ConfigValue_t *cfg) {
    static char topic[96];
    static char value[96];
    if (cfg == NULL) {
        // IP Address
        sprintf(topic, "%s/ip", mqttTopicRoot);
        psclient.publish(topic, ipadd, true);
        return;
    }
    if (ISSYSTEM(cfg)) return;
    sprintf(topic, "%s/%s", mqttTopicRoot, cfg->name);
    switch (cfg->vartype)
    {
    case VARTYPE::ui32:
        if (ISCOLOR(cfg))
            sprintf(value, "{\"V\":\"%u,%u,%u\"}",
                R(cfg->val->ui32.val), G(cfg->val->ui32.val), B(cfg->val->ui32.val));
        else
            sprintf(value, "{\"V\":\"%u\",\"L\":\"%u\",\"H\":\"%u\"}", 
                cfg->val->ui32.val, cfg->val->ui32.min, cfg->val->ui32.max);
        break;
    case VARTYPE::i32:
        sprintf(value, "{\"V\":\"%d\",\"L\":\"%d\",\"H\":\"%d\"}",
                cfg->val->i32.val, cfg->val->i32.min, cfg->val->i32.max);
        break;
    case VARTYPE::f32:
        sprintf(value, "{\"V\":\"%f\",\"L\":\"%f\",\"H\":\"%f\"}",
                cfg->val->f32.val, cfg->val->f32.min, cfg->val->f32.max);
        break;
    case VARTYPE::str:
        sprintf(value, "{\"V\":\"%s\"}", cfg->val->str.str);
        break;
    default:
        strcpy(value,"?");
        break;
    }
    psclient.publish(topic, value, true);
}

void MultiServiceIO::mqttPublish()
{
    mqttPublish(NULL);
    ConfigValue_t *c = config;
    while (c) {
        mqttPublish(c);
        c = c->next;
    }
}

void MultiServiceIO::loop()
{
    WiFiConnected = wifistate >= WIFISTATE::ONLINE;

    // Execute once per Second
    unsigned long mls = millis();
    if (mls - lastMillis >= 1000)
    {
        // 1 Second, fix
        lastMillis += 1000;
        // Serial.print("Debug: Free Memory = ");
        // Serial.println(ESP.getFreeHeap());
        if (lastStoreRequest >= 0)
        {
            if (lastStoreRequest == 0)
            {
                // Store
                Serial.println("Storing Configuration data in SPIFFS");
                storeConfigToFlash();
                // dumpConfig();
                Serial.println("Configuration stored");
            }
            lastStoreRequest--;
        }
    }

    // Execute every 10 ms or slower
    if (mls - txMillis >= 10) {
        // ~ all 10ms
        txMillis = mls+10;
        if (notifyTX) {
            // Should send something
            if (packstart <= packstop) {
                Serial.printf("Sending Packet %d\n", packstart);
                if (buildTXPackage(packstart)) {
                    packstart++;
                    txChar->setValue((uint8_t*)&notify, 20);
                    txChar->notify();
                } else {
                    Serial.println("Notify Serialize Error");
                    notifyTX = false;
                }
            }
            if (packstart > packstop) {
                notifyTX = false; // Done
                Serial.println("Requested Notify Sent completely");
            }
        }
    }

    switch (wifistate)
    {
    case WIFISTATE::UNINITIALIZED:
        // No WLAN specified --> see whether that has changed
        if (strlen(WLAN_SSID.str) > 0)
        {
            // Yes, try to connect
            WiFi.begin(WLAN_SSID.str, WLAN_PWD.str);
            Serial.println("Trying to connect to WiFi AP...");
            wifistate = WIFISTATE::OFFLINE;
        }
        return;
    case WIFISTATE::OFFLINE:
        // WLAN tries to connect --> wait for connection
        if (WiFi.status() == WL_CONNECTED)
        {
            // Must initialize WIFI
            Serial.println("WiFi Connected successfully.");
            IPAddress ip = WiFi.localIP();
            sprintf(ipadd, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            Serial.print("Local IP = ");
            Serial.println(ipadd);
            wifistate = WIFISTATE::ONLINE;
        }
        return;
    case WIFISTATE::ONLINE:
        // WLAN is up and connected --> try to connect to broker
        if (WiFi.status() != WL_CONNECTED)
        {
            // Fall Back to disconnect state
            Serial.println("Wifi Disconnected");
            wifistate = WIFISTATE::OFFLINE;
            return;
        }
        if (strlen(MQTT_BROKER.str) > 0)
        {
            // Try to connect to broker
            Serial.printf("Trying to Connect to Broker %s:%d\n",
                            MQTT_BROKER.str, MQTT_PORT.val);
            psclient.setServer(MQTT_BROKER.str, MQTT_PORT.val);
            global_mio = this;
            psclient.setCallback(&mqtt_callback);
            if ((strlen(MQTT_USER.str) > 0) || (strlen(MQTT_PWD.str) > 0)) {
                Serial.printf("\tCredentials = [%s]:[%s]\n", MQTT_USER.str, MQTT_PWD.str);
                psclient.connect(devName, MQTT_USER.str, MQTT_PWD.str);
            } else
                psclient.connect(devName);
            wifistate = WIFISTATE::MQTTDOCONNECT;
            return;
        }
        return;
    case WIFISTATE::MQTTDOCONNECT:
        // WLAN Connected, but no connection to MQTT Broker yet
        if (WiFi.status() != WL_CONNECTED)
        {
            // Fall back to disconnected
            wifistate = WIFISTATE::OFFLINE;
            psclient.disconnect();
            Serial.println("Wifi Connection lost.");
            return;
        }
        psclient.loop();
        if (psclient.connected())
        {
            // Connection to broker establisched
            Serial.println("Conenction to MQTT broker established");
            wifistate = WIFISTATE::MQTTCONNECTED;
            mqttSubscribe();
            mqttPublish();
            return;
        }
        return;
    case WIFISTATE::MQTTCONNECTED:
        if (WiFi.status() != WL_CONNECTED)
        {
            // Fall back to WiFi disconnected state
            psclient.disconnect();
            wifistate = WIFISTATE::OFFLINE;
            Serial.println("Wifi Connection lost.");
            return;
        }
        psclient.loop();
        if (!psclient.connected())
        {
            // Fall back to PSClient disconnected state
            wifistate = WIFISTATE::ONLINE;
            psclient.disconnect();
            Serial.println("MQTT Connection lost");
            return;
        }
        updateMQTT();
        return;
    }
}

void MultiServiceIO::updateMQTT() {
    // Check for updated data
    ConfigValue_t *cfg = config;
    while (cfg)
    {
        if (!ISSYSTEM(cfg))
        {
            if (ISMQTT(cfg->val->any) || ISBLE(cfg->val->any))
            {
                // Needs update
                mqttPublish(cfg);
                CLEARMQTT(cfg->val->any);
                CLEARBLE(cfg->val->any);
            }
        }
        cfg = cfg->next;
    }
}

void MultiServiceIO::storeRequired()
{
    lastStoreRequest = 10; // Store after 10 seconds
}

    // EOF