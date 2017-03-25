# Third Party Components

- ESPCan Driver 
  - Base CAN Driver [forked from Thomas Barth](https://github.com/ThomasBarth/ESP32-CAN-Driver)
  - General [Component CAN Driver Pack](https://github.com/ESP32DE/ESP32-CAN-Driver/tree/Component_CAN_Driver_Pack) Work for ESP-IDF with menuconfig from [rudi ;-)](http://esp32.de)

 * version 0.1
 * A basic CAN driver for the Espressif ESP32 by [Thomas Barth](http://barth-dev.de)
 *
 *		Change Log from 22 March 2017
 * 		version 0.1_third_parts by esp32de [rudi ;-)](http://esp32.de)
 *  		- modified for esp-idf
 *  		- modified for component driver mode
 *			- menuconfig file created
 * 			- cretaed component.mk for menuconfig and component version 
 *  		- modified and expanded demo code for using with menuconfig
 *			- modified code for using it for Linux and Windows user
 *
 *			menuconfig config defines that you can use in the code:
 *				ESPCan activated:
 *					- CONFIG_ESPCAN ( bool ) 
 * 				baudrade 100,125, 250, 500, 800, 1000 and user: 
 *					- CONFIG_CAN_SPEED_100KBPS 
 *					- CONFIG_CAN_SPEED_125KBPS
 *					- CONFIG_CAN_SPEED_250KBPS
 *					- CONFIG_CAN_SPEED_500KBPS
 *					- CONFIG_CAN_SPEED_800KBPS
 *					- CONFIG_CAN_SPEED_1000KBPS
 *					- CONFIG_CAN_SPEED_USER_KBPS
 *				user can speed
 *					- CONFIG_CAN_SPEED_USER_KBPS ( bool )
 *						-CONFIG_CAN_SPEED_USER_KBPS_VAL (value)
 *				gpio pins for CANRx and CANTx
 *					- CONFIG_ESP_CAN_RXD_PIN_NUM
 *					- CONFIG_ESP_CAN_TXD_PIN_NUM 
 *				node id ( CAN identifier )
 *					- CONFIG_ESP_CAN_NODE_ITSELF
 *				enbable/disable send test frames
 *					- CAN_TEST_SENDING_ENABLED
 *					- CAN_TEST_SENDING_DISABLED 
 
  


    