#ifndef __CONFIG_H__
#define __CONFIG_H__

/************************* MQTT Config **************************************/
#define MQTT_TOPIC_ROOT "light"
#define MQTT_DEV_PREFIX "LE"
// Note: MQTT Topic root will be {MQTT_TOPIC_ROOT}/{MQTT_PREFIX}_XX_XX_XX_XX_XX_XX/ with XX..XX beeing the MAC Address

/************************** BLE Access *************************************/
#define BLE_NAME            "LK300"
#define BLE_BASE_UUID       "96a00000-bb09-4cf0-b0aa-2fd28b75f616" 
#define BLE_MFG_NAME        "Thomas Buck"
#define BLE_MODEL_NUMBER    "1"
#define BLE_FW_REV          "1"
#define BLE_HW_REV          "1"
#define BLE_SW_REV          "1"
// Notes: 
// the BLE Serial Number will be the MAC_Address in the form XXXXXXXXXXXX (hex number)
// the BASE UUID can be created individually (e.g. via https://www.uuidgenerator.net/version4)
// Make sure that it has the format XXXX0000-XXXX-XXXX-XXXX-XXXXXXXXXXXX
// Two services will be provided via BLE (apart from the device information service):
// XXXX0000-XXXX-XXXX-XXXX-XXXXXXXXXXXX: Access to the configuration, password protected (see below)
// XXXX1000-XXXX-XXXX-XXXX-XXXXXXXXXXXX: Access to the parametes, not protected at all.

/************************** BLE Write Access Password ****************************/
#define BLE_DEFAULT_PW      "bL3-h4Ck" // Note: This can and should be changed via BLE!
// Note: Even with this password, the WLAN Credentials (Passwords) cannot be retrieved. But they can be changed.

/***************************************************************************/
#endif // __CONFIG_H__
