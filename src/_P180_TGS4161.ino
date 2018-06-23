//#######################################################################################################
//#################################### Plugin 002: Analog ###############################################
//#######################################################################################################

#define PLUGIN_180
#define PLUGIN_ID_180         180
#define PLUGIN_NAME_180       "Analog input - TGS4161"
#define PLUGIN_VALUENAME1_180 "CO2"

boolean Plugin_180_init = false;

// define sensor functions
#include "CO2Sensor.h"
CO2Sensor co2Sensor(A0, 0.99, 100);

boolean Plugin_180(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_180;
        Device[deviceCount].Type = DEVICE_TYPE_ANALOG;
        Device[deviceCount].VType = SENSOR_TYPE_NVALUE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_180);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_180));
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_180_init = true;
        Serial.begin(9600);
        co2Sensor.calibrate();
        addLog(LOG_LEVEL_INFO,"=== TGS4161 Initialized ===");
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (Plugin_180_init)
        {
          String log = F("TGS4161 PPM: ");
          int co2 = co2Sensor.read();
          UserVar[event->BaseVarIndex] = co2;
          log += co2;
          addLog(LOG_LEVEL_INFO,log);
          success = true;
          break;
        }
      }
  }
  return success;
}
