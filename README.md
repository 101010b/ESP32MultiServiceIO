# ESP32MultiServiceIO
Simple C++ Code allowing to provide arbitrary parameters (int/float/string) via BLE, via WLAN(MQTT) and also store them permanently on the local Flash (SPIFFS). It also provides a password protected configuration interface via BLE.

Features:
* Definition of parameters for your embedded app including strings, signed/unsigned integers, RGB Colors, and floats (with min/max limits) 
* Parameters are exposed via BLE to any Smartphone close by (no access limitation)
* Configuration parameters (WLAN SSID, Key) can also be set via BLE after to unlocking via a password 
* Parameters can also be accessed via MQTT on the WLAN.
* MQTT Publishes the Parameters together with their ranges

There is also a simple Android App to configure the device and the parameters via BLE: https://github.com/101010b/ESP32MultiServiceIOConfiguratorAndroidApp

An Example Embedded Project can be found here: https://github.com/101010b/ESP32LedStripExample

The python directory contains a skript that converts the MQTT-Parameters to the required OpenHAB items/things/sitemap files.
