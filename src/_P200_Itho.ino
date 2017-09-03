//#######################################################################################################
//############################## Plugin 200: Itho ventilation unit 868Mhz remote ########################
//#######################################################################################################
// based on ThinkPad's code https://bitbucket.org/frankiepankie/random-scripts/src/master/IthoRemoteESPEasy/_P054_Itho.ino?at=master&fileviewer=file-view-default
// added support for reading remote commands based on the code from supersjimmie mentioned further on.
//
// List of commands:
// JOIN to join ESP8266 with Itho ventilation unit
// LOW - set Itho ventilation unit to lowest speed
// MEDIUM - set Itho ventilation unit to medium speed
// HIGH - set Itho ventilation unit to max speed

// Usage (not case sensitive):
// http://ip/control?cmd=ITHOSEND,join
// http://ip/control?cmd=ITHOSEND,low
// http://ip/control?cmd=ITHOSEND,medium
// http://ip/control?cmd=ITHOSEND,high
// http://ip/control?cmd=ITHOSEND,timer1
// http://ip/control?cmd=ITHOSEND,timer2
// http://ip/control?cmd=ITHOSEND,timer3
//
// Wemos pins
//SCLK - D5
//MISO - D6
//MOSI - D7
//SS - D8
//
//+3.3V op 3.3V, GND op GND.
//D8 naar de laatste pin van de cc1101, pin CSN.
//D7 gaat naar MOSI
//D6 gaat naar MISO
//D5 gaat naar SCK
//

//This code needs the library made by 'supersjimmie': https://github.com/supersjimmie/IthoEcoFanRFT/tree/master/Master/Itho
// A CC1101 868Mhz transmitter is needed
// See https://gathering.tweakers.net/forum/list_messages/1690945 for more information

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
IthoCC1101 rf;
IthoPacket packet;

#define PLUGIN_200
#define PLUGIN_ID_200         200
#define PLUGIN_NAME_200       "Itho ventilation remote"
#define PLUGIN_VALUENAME1_200 "Last Received command"
boolean Plugin_200_init = false;

boolean Plugin_200(byte function, struct EventStruct *event, String& string)
{
	boolean success = false;

	switch (function) {
		case PLUGIN_DEVICE_ADD:
		{
			Device[++deviceCount].Number = PLUGIN_ID_200;
			Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
			Device[deviceCount].VType = SENSOR_TYPE_DIMMER;
			Device[deviceCount].Ports = 0;
			Device[deviceCount].PullUpOption = false;
			Device[deviceCount].InverseLogicOption = false;
			Device[deviceCount].FormulaOption = false;
			Device[deviceCount].DecimalsOnly = true;
			Device[deviceCount].ValueCount = 1;
			Device[deviceCount].SendDataOption = true;
			Device[deviceCount].TimerOption = true;
			Device[deviceCount].TimerOptional = true;
  		Device[deviceCount].GlobalSyncOption = true;
			break;
		}

		case PLUGIN_GET_DEVICENAME:
		{
			string = F(PLUGIN_NAME_200);
			break;
		}

		case PLUGIN_GET_DEVICEVALUENAMES:
		{
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_200));
			break;
		}

		case PLUGIN_INIT:
		{
			rf.init();
			//rf.sendCommand(IthoJoin);
			addLog(LOG_LEVEL_INFO, F("CC1101 868Mhz transmitter initialized"));
			success = true;
			Plugin_200_init = true;
			break;
		}

		case PLUGIN_TEN_PER_SECOND:
      {
        // check for commands send by other remote
				if (Plugin_200_init) {
					if (rf.checkForNewPacket()) {
						String cmd = F("?");
						String log = F("Itho received:  ");
						packet = rf.getLastPacket();
						//show counter
						log += F("counter=");
						log += packet.counter;
						log += F("/");
						//show command
						switch (packet.command) {
			        case IthoLow:
								cmd = F("low\n");
								UserVar[event->BaseVarIndex] = 1;
								break;
			        case IthoMedium:
			        	cmd = F("medium\n");
								UserVar[event->BaseVarIndex] = 2;
			        	break;
			        case IthoFull:
			        	cmd = F("full\n");
								UserVar[event->BaseVarIndex] = 3;
			        	break;
			        case IthoTimer1:
			        	cmd = F("timer1\n");
								UserVar[event->BaseVarIndex] = 4;
		          	break;
			        case IthoTimer2:
			        	cmd = F("timer2\n");
								UserVar[event->BaseVarIndex] = 5;
			        	break;
			        case IthoTimer3:
			        	cmd = F("timer3\n");
								UserVar[event->BaseVarIndex] = 6;
			        	break;
			        case IthoJoin:
			        	cmd = F("join\n");
								UserVar[event->BaseVarIndex] = 7;
			        	break;
			        case IthoLeave:
			        	cmd = F("leave\n");
								UserVar[event->BaseVarIndex] = 8;
			       		break;
							case IthoUnknown:
			        	cmd = F("unknown\n");
								UserVar[event->BaseVarIndex] = 9;
								break;
							}
							// send if output needs to be changed
							event->sensorType = SENSOR_TYPE_DIMMER;
							log += cmd;
							log += "/";
							log += UserVar[event->BaseVarIndex];
							addLog(LOG_LEVEL_INFO, log);
							sendData(event);
						}  // switch (recv) command
					}  // checkfornewpacket
					success = true;
					break;
        }

		case PLUGIN_WRITE: {
			String tmpString = string;
			String cmd = parseString(tmpString, 1);
			String param1 = parseString(tmpString, 2);
			if (cmd.equalsIgnoreCase(F("ITHOSEND"))) {
				if (param1.equalsIgnoreCase(F("join"))) {
					//rf.sendCommand(IthoJoin);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'join' to Itho unit"));
					printWebString += F("Sent command for 'join' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("leave"))) {
					//rf.sendCommand(IthoLeave);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'leave' to Itho unit"));
					printWebString += F("Sent command for 'leave' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("low"))) {
					//rf.sendCommand(IthoLow);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'low speed' to Itho unit"));
					printWebString += F("Sent command for 'low speed' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("medium")))	{
					//rf.sendCommand(IthoMedium);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'medium speed' to Itho unit"));
					printWebString += F("Sent command for 'medium speed' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("high"))) {
					//rf.sendCommand(IthoFull);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'full speed' to Itho unit"));
					printWebString += F("Sent command for 'full speed' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("timer1")))	{
					//rf.sendCommand(IthoTimer1);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 1' to Itho unit"));
					printWebString += F("Sent command for 'timer 1' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("timer2"))) {
					//rf.sendCommand(IthoTimer2);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 2' to Itho unit"));
					printWebString += F("Sent command for 'timer 2' to Itho unit");
					success = true;
				}
				else if (param1.equalsIgnoreCase(F("timer3")))	{
					//rf.sendCommand(IthoTimer3);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 3' to Itho unit"));
					printWebString += F("Sent command for 'timer 3' to Itho unit");
					success = true;
				}
			}
		}

	break;
	}
	return success;
}
