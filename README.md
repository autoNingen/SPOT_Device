# SPOT Device & Coordinator
The End device should have I2C pins set to SDA = 6, SCL = 7
Coordinator can only accept packets in float (uint32_t) with values from 10-50. (Might be able to adjust in future)
For now, data is validated on each end in order to get accurate readings in mm.

Note:
 * This board means the board (e.g. ESP32-H2) loaded with `Zigbee_Thermostat` example.
 * The remote board means the board (e.g. ESP32-H2) loaded with `Zigbee_Temperature_Sensor` example.

Functions:
 * By clicking the button (BOOT) on this board, this board will read temperature value, temperature measurement range and temperature tolerance from the remote board. Also, this board will configure the remote board to report the measured temperature value every 10 seconds or every 2 degree changes.


* Before Compile/Verify, select the correct board: `Tools -> Board`.
* Select the Coordinator Zigbee mode: `Tools -> Zigbee mode: Zigbee ZCZR (coordinator/router)`.
* Select Partition Scheme for Zigbee: `Tools -> Partition Scheme: Zigbee 4MB with spiffs`.
* Select the COM port: `Tools -> Port: xxx where the `xxx` is the detected COM port.
* Optional: Set debug level to verbose to see all logs from Zigbee stack: `Tools -> Core Debug Level: Verbose`.

## Troubleshooting

If the End device flashed with the example `Zigbee_Temperature_Sensor` is not connecting to the coordinator, erase the flash of the End device before flashing the example to the board. It is recommended to do this if you re-flash the coordinator.
You can do the following:

* In the Arduino IDE go to the Tools menu and set `Erase All Flash Before Sketch Upload` to `Enabled`.
* In the `Zigbee_Temperature_Sensor` example sketch call `Zigbee.factoryReset();`.

By default, the coordinator network is closed after rebooting or flashing new firmware.
To open the network you have 2 options:

* Open network after reboot by setting `Zigbee.setRebootOpenNetwork(time);` before calling `Zigbee.begin();`.
* In application you can anytime call `Zigbee.openNetwork(time);` to open the network for devices to join.

