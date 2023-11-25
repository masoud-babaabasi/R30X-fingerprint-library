/*************************************************************************
 *
 * finger print library
 * author  :	Masoud Babaabasi
 * October 2023
 *
 *
 *
 **************************************************************************/
#ifndef R30X_FPS_H
#define R30X_FPS_H
#include "stdint.h"
#include "string.h"
//=========================================================================//
//Response codes from FPS to the commands sent to it
//FPS = Fingerprint Scanner

#define FPS_RESP_OK                      0x00   //command executed successfully
#define FPS_RESP_RECIEVEERR              0x01   //packet receive error
#define FPS_RESP_NOFINGER                0x02   //no finger detected
#define FPS_RESP_ENROLLFAIL              0x03   //failed to enroll the finger
#define FPS_RESP_OVERDISORDERFAIL        0x04   //failed to generate character file due to over-disorderly fingerprint image
#define FPS_RESP_OVERWETFAIL             0x05   //failed to generate character file due to over-wet fingerprint image
#define FPS_RESP_OVERDISORDERFAIL2       0x06   //failed to generate character file due to over-disorderly fingerprint image
#define FPS_RESP_FEATUREFAIL             0x07   //failed to generate character file due to over-wet fingerprint image
#define FPS_RESP_DONOTMATCH              0x08   //fingers do not match
#define FPS_RESP_NOTFOUND                0x09   //no valid match found
#define FPS_RESP_ENROLLMISMATCH          0x0A   //failed to combine character files (two character files (images) are used to create a template)
#define FPS_RESP_BADLOCATION             0x0B   //addressing PageID is beyond the finger library
#define FPS_RESP_INVALIDTEMPLATE         0x0C   //error when reading template from library or the template is invalid
#define FPS_RESP_TEMPLATEUPLOADFAIL      0x0D   //error when uploading template
#define FPS_RESP_PACKETACCEPTFAIL        0x0E   //module can not accept more packets
#define FPS_RESP_IMAGEUPLOADFAIL         0x0F   //error when uploading image
#define FPS_RESP_TEMPLATEDELETEFAIL      0x10   //error when deleting template
#define FPS_RESP_DBCLEARFAIL             0x11   //failed to clear fingerprint library
#define FPS_RESP_WRONGPASSOWRD           0x13   //wrong password
#define FPS_RESP_IMAGEGENERATEFAIL       0x15   //fail to generate the image due to lackness of valid primary image
#define FPS_RESP_FLASHWRITEERR           0x18   //error when writing flash
#define FPS_RESP_NODEFINITIONERR         0x19   //no definition error
#define FPS_RESP_INVALIDREG              0x1A   //invalid register number
#define FPS_RESP_INCORRECTCONFIG         0x1B   //incorrect configuration of register
#define FPS_RESP_WRONGNOTEPADPAGE        0x1C   //wrong notepad page number
#define FPS_RESP_COMPORTERR              0x1D   //failed to operate the communication port
#define FPS_RESP_INVALIDREG              0x1A   //invalid register number
#define FPS_RESP_SECONDSCANNOFINGER      0x41   //secondary fingerprint scan failed due to no finger
#define FPS_RESP_SECONDENROLLFAIL        0x42   //failed to enroll second fingerprint
#define FPS_RESP_SECONDFEATUREFAIL       0x43   //failed to generate character file due to lack of enough features
#define FPS_RESP_SECONDOVERDISORDERFAIL  0x44   //failed to generate character file due to over-disorderliness
#define FPS_RESP_DUPLICATEFINGERPRINT    0x45   //duplicate fingerprint

//-------------------------------------------------------------------------//
//Received packet verification status codes from host device

#define FPS_RX_OK                       0x00  //when the response is correct
#define FPS_RX_BADPACKET                0x01  //if the packet received from FPS is badly formatted
#define FPS_RX_WRONG_ADDRESS			0x02  // if address of received packet is wrong
#define FPS_RX_WRONG_RESPONSE           0x03  //unexpected response
#define FPS_RX_TIMEOUT                  0x04  //when no response was received
#define FPS_RX_WRONG_CHECKSUM           0x05  //when no response was received

//-------------------------------------------------------------------------//
//Packet IDs

#define FPS_ID_STARTCODE              0xEF01
#define FPS_ID_STARTCODE_H	          0xEF
#define FPS_ID_STARTCODE_L	          0x01

#define FPS_ID_COMMANDPACKET          0x01
#define FPS_ID_DATAPACKET             0x02
#define FPS_ID_ACKPACKET              0x07
#define FPS_ID_ENDDATAPACKET          0x08

//-------------------------------------------------------------------------//
//Command codes

#define FPS_CMD_SCANFINGER					 0x01    //scans the finger and collect finger image
#define FPS_CMD_IMAGETOCHARACTER			 0x02    //generate char file from a single image and store it to one of the buffers
#define FPS_CMD_MATCHTEMPLATES				 0x03    //match two fingerprints precisely
#define FPS_CMD_SEARCHLIBRARY				 0x04    //search the fingerprint library
#define FPS_CMD_GENERATETEMPLATE			 0x05    //combine both character buffers and generate a template
#define FPS_CMD_STORETEMPLATE				 0x06    //store the template on one of the buffers to flash memory
#define FPS_CMD_LOADTEMPLATE				 0x07    //load a template from flash memory to one of the buffers
#define FPS_CMD_EXPORTTEMPLATE				 0x08    //export a template file from buffer to computer
#define FPS_CMD_IMPORTTEMPLATE				 0x09    //import a template file from computer to sensor buffer
#define FPS_CMD_EXPORTIMAGE					 0x0A    //export fingerprint image from buffer to computer
#define FPS_CMD_IMPORTIMAGE					 0x0B    //import an image from computer to sensor buffer
#define FPS_CMD_DELETETEMPLATE				 0x0C    //delete a template from flash memory
#define FPS_CMD_CLEARLIBRARY				 0x0D    //clear fingerprint library
#define FPS_CMD_SETSYSPARA					 0x0E    //set system configuration register
#define FPS_CMD_READSYSPARA					 0x0F    //read system configuration register
#define FPS_CMD_SETPASSWORD					 0x12    //set device password
#define FPS_CMD_VERIFYPASSWORD				 0x13    //verify device password
#define FPS_CMD_GETRANDOMCODE				 0x14    //get random code from device
#define FPS_CMD_SETDEVICEADDRESS			 0x15    //set 4 byte device address
#define FPS_CMD_READALL_SYSPARA				 0x16   //read system configurations
#define FPS_CMD_PORTCONTROL					 0x17    //enable or disable comm port
#define FPS_CMD_WRITENOTEPAD				 0x18    //write to device notepad
#define FPS_CMD_READNOTEPAD					 0x19    //read from device notepad
#define FPS_CMD_HISPEEDSEARCH				 0x1B    //highspeed search of fingerprint
#define FPS_CMD_TEMPLATECOUNT				 0x1D    //read total template count
#define FPS_CMD_SCANANDRANGESEARCH			 0x32    //read total template count
#define FPS_CMD_SCANANDFULLSEARCH			 0x34    //read total template count

#define FPS_DEFAULT_TIMEOUT                 1000	//UART reading timeout in milliseconds
#define FPS_DEFAULT_BAUDRATE                57600  //9600*6
#define FPS_DEFAULT_RX_DATA_LENGTH          16   //the max length of data in a received packet
#define FPS_DEFAULT_SECURITY_LEVEL          3    //the threshold at which the fingerprints will be matched
#define FPS_DEFAULT_PASSWORD                0x00000000
#define FPS_DEFAULT_ADDRESS                 0xFFFFFFFF
#define FPS_BAD_VALUE                       0x1FU //some bad value or paramter was delivered

typedef struct {
	//common parameters
	  uint32_t devicePassword; //32-bit single value version of password (L = long)
	  uint32_t deviceAddress;  //module's address
	  char deviceName[32];

	  uint16_t statusRegister;  //contents of the FPS status register
	  uint16_t securityLevel;  //threshold level for fingerprint matching
	  uint16_t dataPacketLengthCode;
	  uint16_t dataPacketLength; //the max length of data in packet. can be 32, 64, 128 or 256
	  uint16_t baudMultiplier;  //value between 1-12
	  uint32_t deviceBaudrate;  //UART speed (9600 * baud multiplier)

	  //receive packet parameters
	  uint8_t	rxPacketType; //type of packet
	  uint8_t	rxConfirmationCode; //the return codes from the FPS
	  uint8_t	rxDataBuffer[FPS_DEFAULT_RX_DATA_LENGTH]; //packet data buffer
	  uint32_t	rxDataBufferLength;  //the length of the data only. this doesn't include instruction or confirmation code

	  uint16_t fingerId; //location of fingerprint in the library
	  uint16_t matchScore;  //the match score of comparison of two fingerprints
	  uint16_t templateCount; //total number of fingerprint templates in the library

	  uint32_t(*read)(uint8_t* pBuf, uint16_t BytesToRead , uint16_t timout); // return number of bytes read
	  uint32_t(*write)(uint8_t* pBuff, uint16_t BytesToWrite,uint16_t timout); // returns number of bytes written
	  uint8_t (*initializePort) (uint32_t baud); // retun 0 on success
	  uint8_t(*deinitializePort) (void);
}__FPS;
  
int8_t	R30X_init(__FPS *stream, uint32_t password , uint32_t address );
void	resetParameters (__FPS *stream); //initialize and reset and all parameters
uint8_t verifyPassword (__FPS *stream,uint32_t password ); //verify the user supplied password
uint8_t setPassword (__FPS *stream,uint32_t password);  //set FPS password
uint8_t setAddress (__FPS *stream,uint32_t address );  //set FPS address
uint8_t setBaudrate (__FPS *stream,uint32_t baud);  //set UART baudrate, default is 57000
uint8_t setSecurityLevel (__FPS *stream,uint8_t level); //set the threshold for fingerprint matching
uint8_t setDataLength (__FPS *stream,uint16_t length); //set the max length of data in a packet
uint8_t portControl (__FPS *stream,uint8_t value);  //turn the comm port on or off
void    sendPacket (__FPS *stream, uint8_t command, uint8_t* data , uint16_t dataLength); //assemble and send packets to FPS
uint8_t receivePacket (__FPS *stream, uint32_t timeout); //receive packet from FPS
uint8_t readSysPara (__FPS *stream); //read FPS system configuration
uint8_t captureAndRangeSearch (__FPS *stream,uint16_t captureTimeout, uint16_t startId, uint16_t count); //scan a finger and search a range of locations
uint8_t captureAndFullSearch (__FPS *stream);  //scan a finger and search the entire library
uint8_t generateImage (__FPS *stream); //scan a finger, generate an image and store it in the buffer
uint8_t exportImage (__FPS *stream); //export a fingerprint image from the sensor to the computer
uint8_t importImage (__FPS *stream, uint8_t* dataBuffer);  //import a fingerprint image from the computer to sensor
uint8_t generateCharacter (__FPS *stream, uint8_t bufferId); //generate character file from image
uint8_t generateTemplate (__FPS *stream);  //combine the two character files and generate a single template
uint8_t exportCharacter (__FPS *stream, uint8_t bufferId); //export a character file from the sensor to computer
uint8_t importCharacter (__FPS *stream, uint8_t bufferId, uint8_t* dataBuffer);  //import a character file to the sensor from computer
uint8_t saveTemplate (__FPS *stream, uint8_t bufferId, uint16_t location);  //store the template in the buffer to a location in the library
uint8_t loadTemplate (__FPS *stream, uint8_t bufferId, uint16_t location); //load a template from library to one of the buffers
uint8_t deleteTemplate (__FPS *stream, uint16_t startLocation, uint16_t count);  //delete a set of templates from library
uint8_t clearLibrary (__FPS *stream);  //delete all templates from library
uint8_t matchTemplates (__FPS *stream);  //match the templates stored in the two character buffers
uint8_t searchLibrary (__FPS *stream, uint8_t bufferId, uint16_t startLocation, uint16_t count); //search the library for a template stored in the buffer
uint8_t getTemplateCount (__FPS *stream);  //get the total no. of templates in the library
uint8_t generateRandomNumber(__FPS* stream, uint32_t* random);
uint8_t getImage(__FPS* stream, uint8_t* image_buffer);
#endif

/********************************END OF FILE*****************************************************/
