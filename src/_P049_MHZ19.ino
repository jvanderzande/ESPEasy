/*

  This plug in is written by Dmitry (rel22 ___ inbox.ru)
  Plugin is based upon SenseAir plugin by Daniel Tedenljung info__AT__tedenljungconsulting.com
  Additional features based on https://geektimes.ru/post/285572/ by Gerben (infernix__AT__gmail.com)

  This plugin reads the CO2 value from MH-Z19 NDIR Sensor
  DevicePin1 - is RX for ESP
  DevicePin2 - is TX for ESP
*/

#define PLUGIN_049
#define PLUGIN_ID_049         49
#define PLUGIN_NAME_049       "Gases - CO2 MH-Z19(b)"
#define PLUGIN_VALUENAME1_049 "PPM"
#define PLUGIN_VALUENAME2_049 "Temperature" // Temperature in C
#define PLUGIN_VALUENAME3_049 "U"   // Undocumented, minimum measurement per time period?
#define PLUGIN_READ_TIMEOUT   3000

boolean Plugin_049_init = false;
// Default of the sensor is to run ABC
boolean Plugin_049_ABC_Disable = false;
boolean Plugin_049_ABC_MustApply = false;

#include <SoftwareSerial.h>
SoftwareSerial *Plugin_049_SoftSerial;

// 9-bytes CMD PPM read command
byte mhzCmdReadPPM[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
byte mhzResp[9];    // 9 bytes bytes response
byte mhzCmdCalibrateZero[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78};
byte mhzCmdABCEnable[9] = {0xFF,0x01,0x79,0xA0,0x00,0x00,0x00,0x00,0xE6};
byte mhzCmdABCDisable[9] = {0xFF,0x01,0x79,0x00,0x00,0x00,0x00,0x00,0x86};
byte mhzCmdReset[9] = {0xFF,0x01,0x8d,0x00,0x00,0x00,0x00,0x00,0x72};
byte mhzCmdMeasurementRange1000[9] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x03,0xE8,0x7B};
byte mhzCmdMeasurementRange2000[9] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x07,0xD0,0x8F};
byte mhzCmdMeasurementRange3000[9] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x0B,0xB8,0xA3};
byte mhzCmdMeasurementRange5000[9] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x13,0x88,0xCB};

enum
{
  ABC_enabled  = 0x01,
  ABC_disabled = 0x02
};

boolean Plugin_049(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_049;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_AIRQUALITY;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_049);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_049));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_049));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_049));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[2] = { F("Normal"), F("ABC disabled") };
        int optionValues[2] = { ABC_enabled, ABC_disabled };
        addFormSelector(string, F("Auto Base Calibration"), F("plugin_049_abcdisable"), 2, options, optionValues, choice);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        const int formValue = getFormItemInt(F("plugin_049_abcdisable"));
        boolean new_ABC_disable = (formValue == ABC_disabled);
        if (Plugin_049_ABC_Disable != new_ABC_disable) {
          // Setting changed in the webform.
          Plugin_049_ABC_MustApply = true;
          Plugin_049_ABC_Disable = new_ABC_disable;
        }
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = formValue;
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (Plugin_049_ABC_Disable) {
          // No guarantee the correct state is active on the sensor after reboot.
          Plugin_049_ABC_MustApply = true;
        }
        Plugin_049_ABC_Disable = Settings.TaskDevicePluginConfig[event->TaskIndex][0] == ABC_disabled;
        Plugin_049_SoftSerial = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex]);
        Plugin_049_SoftSerial->begin(9600);
        addLog(LOG_LEVEL_INFO, F("MHZ19: Init OK "));

        //delay first read, because hardware needs to initialize on cold boot
        //otherwise we get a weird value or read error
        timerSensor[event->TaskIndex] = millis() + 15000;

        Plugin_049_init = true;
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);

        if (command == F("mhzcalibratezero"))
        {
          Plugin_049_SoftSerial->write(mhzCmdCalibrateZero, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Calibrated zero point!"));
          success = true;
        }

        if (command == F("mhzreset"))
        {
          Plugin_049_SoftSerial->write(mhzCmdReset, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent sensor reset!"));
          success = true;
        }

        if (command == F("mhzabcenable"))
        {
          Plugin_049_SoftSerial->write(mhzCmdABCEnable, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent sensor ABC Enable!"));
          success = true;
        }

        if (command == F("mhzabcdisable"))
        {
          Plugin_049_SoftSerial->write(mhzCmdABCDisable, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent sensor ABC Disable!"));
          success = true;
        }

        if (command == F("mhzmeasurementrange1000"))
        {
          Plugin_049_SoftSerial->write(mhzCmdMeasurementRange1000, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent measurement range 0-1000PPM!"));
          success = true;
        }

        if (command == F("mhzmeasurementrange2000"))
        {
          Plugin_049_SoftSerial->write(mhzCmdMeasurementRange2000, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent measurement range 0-2000PPM!"));
          success = true;
        }

        if (command == F("mhzmeasurementrange3000"))
        {
          Plugin_049_SoftSerial->write(mhzCmdMeasurementRange3000, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent measurement range 0-3000PPM!"));
          success = true;
        }

        if (command == F("mhzmeasurementrange5000"))
        {
          Plugin_049_SoftSerial->write(mhzCmdMeasurementRange5000, 9);
          addLog(LOG_LEVEL_INFO, F("MHZ19: Sent measurement range 0-5000PPM!"));
          success = true;
        }
        break;

      }

    case PLUGIN_READ:
      {

        if (Plugin_049_init)
        {
          //send read PPM command
          int nbBytesSent = Plugin_049_SoftSerial->write(mhzCmdReadPPM, 9);
          if (nbBytesSent != 9) {
            String log = F("MHZ19: Error, nb bytes sent != 9 : ");
              log += nbBytesSent;
              addLog(LOG_LEVEL_INFO, log);
          }
          success = false;
        }
      }
    case PLUGIN_ONCE_A_SECOND:
      {
        // Check if response is available
        if (Plugin_049_SoftSerial->available() == 0) {
          success = false;
        } else {
          // get response from MH-Z19
          memset(mhzResp, 0, sizeof(mhzResp));
          long start = millis();
          int counter = 0;
          int skipped = 0;
          String log = F("MHZ19: skipped unknown response: ");
          mhzResp[counter] = Plugin_049_SoftSerial->read();
          int i;
          counter++;
          while (((millis() - start) < PLUGIN_READ_TIMEOUT) && (counter < 9)) {
            if (Plugin_049_SoftSerial->available() > 0) {
              mhzResp[counter] = Plugin_049_SoftSerial->read();
              // check for valid second byte responses after FF
              if ( mhzResp[0] == 0xFF &&
                    ( mhzResp[1] == 0x79 || mhzResp[1] == 0x86 || mhzResp[1] == 0x87 || mhzResp[1] == 0x8d || mhzResp[1] == 0x99) ) {
                counter++;
              } else {
                // just ignore this byte and shift
                log += String(mhzResp[0]); log += "/";
                mhzResp[0] = mhzResp[1];
                skipped++;
              }
            } else {
              delay(10);
            }
          }
          if (skipped > 0) {
            addLog(LOG_LEVEL_INFO, log);
          }
          if (counter < 9){
            String log = F("MHZ19: Error, timeout while trying to read. Received:");
            log += " bytes read  => ";for (i = 0; i < counter; i++) {log += mhzResp[i];log += "/" ;}
            addLog(LOG_LEVEL_INFO, log);
          }

          unsigned int ppm = 0;
          signed int temp = 0;
          unsigned int s = 0;
          float u = 0;
          byte crc = 0;
          for (i = 1; i < 8; i++) crc+=mhzResp[i];
              crc = 255 - crc;
              crc++;

          log = F("MHZ19: ");
          //--- Only needed when debugging to show all received information details
          //log += "counter:";
          //log += String(counter);
          //log += " crc:";
          //log += String(crc); log += "/"; log += String(mhzResp[8]);
          //log += " bytes read  => ";for (i = 0; i < 9; i++) {log += mhzResp[i];log += "/" ;}
          //log += " > ";
          //---
          if ( !(mhzResp[8] == crc) ) {
             log += F("Read error : CRC = ");
             log += String(crc); log += " / "; log += String(mhzResp[8]);
             log += " bytes read  => ";for (i = 0; i < 9; i++) {log += mhzResp[i];log += "/" ;}
             success = false;
             addLog(LOG_LEVEL_ERROR, log);
             break;

          // Process responses to ReadPPM
          } else if (mhzResp[1] == 0x86)  {
              //  counter:9 crc:174/174 bytes read  => 255/134/1/138/65/0/0/0/174/
              //     > PPM value: 394 Temp/S/U values: 25/0/0.00
              //calculate CO2 PPM
              unsigned int mhzRespHigh = (unsigned int) mhzResp[2];
              unsigned int mhzRespLow = (unsigned int) mhzResp[3];
              ppm = (256*mhzRespHigh) + mhzRespLow;

              // set temperature (offset 40)
              unsigned int mhzRespTemp = (unsigned int) mhzResp[4];
              temp = mhzRespTemp - 40;

              // calculate 'u' value
              unsigned int mhzRespUHigh = (unsigned int) mhzResp[6];
              unsigned int mhzRespULow = (unsigned int) mhzResp[7];
              u = (256*mhzRespUHigh) + mhzRespULow;

              UserVar[event->BaseVarIndex] = (float)ppm;
              UserVar[event->BaseVarIndex + 1] = (float)temp;
              UserVar[event->BaseVarIndex + 2] = (float)u;
              if (Plugin_049_ABC_MustApply) {
                // Send ABC enable/disable command based on the desired state.
                if (Plugin_049_ABC_Disable) {
                    Plugin_049_SoftSerial->write(mhzCmdABCDisable, 9);
                    addLog(LOG_LEVEL_INFO, F("MHZ19: Sent sensor ABC Disable!"));
                } else {
                    Plugin_049_SoftSerial->write(mhzCmdABCEnable, 9);
                    addLog(LOG_LEVEL_INFO, F("MHZ19: Sent sensor ABC Enable!"));
                }
                  Plugin_049_ABC_MustApply = false;
              }
              success = true;
              // Log values in all cases
              log += F("PPM value: ");
              log += ppm;
              log += F(" Temp/U: ");
              log += temp;
              log += F("/");
              log += u;
              addLog(LOG_LEVEL_INFO, log);
              sendData(event);  // send update
              break;

          } else if (mhzResp[1] == 0x87)  {
              addLog(LOG_LEVEL_INFO, F("Received CalibrateZero acknowledgment!"));
              success = false;
              break;

          } else if (mhzResp[1] == 0x79)  {
              addLog(LOG_LEVEL_INFO, F("Received ABC Enable/Disable acknowledgment!"));
              success = false;
              break;

          } else if (mhzResp[1] == 0x87)  {
              addLog(LOG_LEVEL_INFO, F("MHZ19: Received Reset acknowledgment!"));
              success = false;
              break;

          } else if (mhzResp[1] == 0x99)  {
              addLog(LOG_LEVEL_INFO, F("Received MeasurementRange acknowledgment!"));
              success = false;
              break;

          // log verbosely anything else that the sensor reports
          } else {
              log += F("Unknown response:");
              log += " bytes read  => ";for (i = 0; i < 9; i++) {log += mhzResp[i];log += "/" ;}
              addLog(LOG_LEVEL_INFO, log);
              success = false;
              break;
          }
        }
        break;
      }
    }
    return success;
  }
