/*************************************************************************
 *
 * finger print library
 * author  :	Masoud Babaabasi
 * October 2023
 *
 *
 *
 **************************************************************************/

#include "R30X_FPS.h"
#ifdef __GNUC__
extern void HAL_Delay(uint32_t Delay);
#endif

static delay_1ms(void) {
#ifdef __GNUC__
    HAL_Delay(1);
#elif defined _MSC_VER
    for (int i = 0; i < 100000; i++) {}
#endif
}

/*
*   @brief: initialize
*   @parameter: pointer to finger print structure
*   @return: return 0 if intialize is ok if not OK return negetive value
*
*/
int8_t R30X_init(__FPS *stream, uint32_t password , uint32_t address ){
  stream->deviceAddress = address;
  resetParameters(stream);  //initialize and reset and all parameters
  if (stream->initializePort(stream->deviceBaudrate) != FPS_RESP_OK) {
      stream->deinitializePort();
      return -1;
  }
  if (verifyPassword(stream, password) != FPS_RESP_OK) {
      stream->deinitializePort();
      return -2;
  }
  stream->devicePassword = password;
  readSysPara(stream);
  return FPS_RESP_OK;
}

/*
*   @brief: reset all parameters to default values
*   @parameter: pointer to finger print structure
*   @return: none
*  
*/
void resetParameters (__FPS *stream) {
  stream->deviceBaudrate = FPS_DEFAULT_BAUDRATE;  //this will be later altered by begin()
  stream->baudMultiplier = (uint16_t)(FPS_DEFAULT_BAUDRATE / 9600);
  stream->securityLevel = FPS_DEFAULT_SECURITY_LEVEL;  //threshold level for fingerprint matching
  stream->dataPacketLength = FPS_DEFAULT_RX_DATA_LENGTH;

  stream->rxPacketType = FPS_ID_COMMANDPACKET; //type of packet
  stream->rxConfirmationCode = FPS_CMD_VERIFYPASSWORD; //
  stream->rxDataBufferLength = 0;
  memset(stream->rxDataBuffer, 0, FPS_DEFAULT_RX_DATA_LENGTH);
  memset(stream->deviceName, 0, 32);
  stream->fingerId = 0; //initialize them
  stream->matchScore = 0;
  stream->templateCount = 0;
}
/*
*   @brief: send fingerprint instruction packet
*   @parameter: pointer to finger print structure
*   @parameter: command to be performed by module
*   @parameter: optianal data( in case of NO data must be NULL or 0 )
*   @parameter: length of optional data( if no data it is don't care )
*   @return: none
*
*/
void sendPacket (__FPS *stream, uint8_t command, uint8_t* data , uint16_t dataLength) {
    uint8_t packet[10];
    uint16_t packet_length = dataLength + 3;// 2 bytes checksum and one byte command
    uint16_t checksum = 0;
    uint8_t ckeck_arr[2];
    packet[0] = FPS_ID_STARTCODE_H;
    packet[1] = FPS_ID_STARTCODE_L;
    packet[2] = (stream->deviceAddress >> 24) & 0xff;
    packet[3] = (stream->deviceAddress >> 16) & 0xff;
    packet[4] = (stream->deviceAddress >> 8) & 0xff;
    packet[5] = (stream->deviceAddress ) & 0xff;
    packet[6] = FPS_ID_COMMANDPACKET;
    packet[7] = (packet_length >> 8) & 0xff;
    packet[8] = (packet_length ) & 0xff;
    packet[9] = command;
    
    checksum = packet[6] + packet[7] + packet[8] + packet[9];
    if (data != NULL) {
        for (uint16_t i = 0; i < dataLength; i++) {
            checksum += data[i];
        }
    }
    ckeck_arr[0] = (checksum >> 8) & 0xff;
    ckeck_arr[1] = (checksum) & 0xff;

    stream->write(packet,10,5);
    if (data != NULL) {
        stream->write(data, dataLength, 250);
    }
  stream->write(ckeck_arr,2,5);
}
/*
*   @brief: receive fingerprint instruction packet
*   @parameter: pointer to finger print structure
*   @parameter: timeout
*   @return: if packet recieded successfully FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t receivePacket (__FPS *stream, uint32_t timeout) {
  uint8_t   serialBuffer[10] = {0};
  uint32_t address;
  uint32_t time = 0;
  uint16_t checksum = 0;

  uint16_t read_bytes = stream->read(serialBuffer, 9, 1);
  while (read_bytes < 9) {
      read_bytes += stream->read(serialBuffer + read_bytes, 9 - read_bytes, 1);
      time++;
      if (time == timeout) return FPS_RX_TIMEOUT;
      delay_1ms();
  }
  if(serialBuffer[0] != FPS_ID_STARTCODE_H || serialBuffer[1] != FPS_ID_STARTCODE_L) return FPS_RX_BADPACKET;
  address = ((uint32_t)serialBuffer[2] << 24) | ((uint32_t)serialBuffer[3] << 16) | ((uint32_t)serialBuffer[4] << 8) | ((uint32_t)serialBuffer[5]);
  if (address != stream->deviceAddress) return FPS_RX_WRONG_ADDRESS;
  if (serialBuffer[6] != FPS_ID_DATAPACKET && serialBuffer[6] != FPS_ID_ACKPACKET && serialBuffer[6] != FPS_ID_ENDDATAPACKET) {
      return FPS_RX_WRONG_RESPONSE;
  }

  stream->rxPacketType = serialBuffer[6];
  stream->rxDataBufferLength = serialBuffer[7] << 8 | serialBuffer[8];
  stream->rxDataBufferLength -= 3;

  checksum = serialBuffer[6] + serialBuffer[7] + serialBuffer[8];
  // read confimation code
  time = 0;
  while (stream->read(&stream->rxConfirmationCode, 1, 1) < 1) {
      time++;
      delay_1ms();
      if( time == timeout) return FPS_RX_TIMEOUT;
  }
  checksum += stream->rxConfirmationCode;
  //read data
  if (stream->rxDataBufferLength) {
      time = 0;
      read_bytes = stream->read(stream->rxDataBuffer, stream->rxDataBufferLength, timeout);
      while (read_bytes < stream->rxDataBufferLength) {
          read_bytes += stream->read(stream->rxDataBuffer + read_bytes, stream->rxDataBufferLength - read_bytes, 10);
          time += 10;
          if (time == timeout) return FPS_RX_TIMEOUT;
          delay_1ms();
      }
  }
  // read ckecksum
  time = 0;
  read_bytes = stream->read(serialBuffer, 2, 2);
  while (read_bytes < 2) {
      read_bytes += stream->read(serialBuffer + read_bytes, 2 - read_bytes, 1);
      time ++;
      if (time == timeout) return FPS_RX_TIMEOUT;
      delay_1ms();
  }
  for (uint16_t i = 0; i < stream->rxDataBufferLength; i++) checksum += stream->rxDataBuffer[i];
  if (checksum != (serialBuffer[0] << 8 | serialBuffer[1])) return FPS_RX_WRONG_CHECKSUM;
  return FPS_RX_OK;
}
/*
*   @brief: receive fingerprint data packet, data won't be store in stream->rxDataBuffer like receivePacket but in provided receive_buffer
*   @parameter: pointer to finger print structure
*   @parameter: timeout
*   @return: if packet recieded successfully FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t receiveDataPacket(__FPS* stream, uint8_t *receive_buffer, uint16_t* receive_length,uint32_t timeout) {
    uint8_t   serialBuffer[10] = { 0 };
    uint32_t address;
    uint32_t time = 0;
    uint16_t checksum = 0;

    uint16_t read_bytes = stream->read(serialBuffer, 9, 1);
    while (read_bytes < 9) {
        read_bytes += stream->read(serialBuffer + read_bytes, 9 - read_bytes, 1);
        time++;
        if (time == timeout) return FPS_RX_TIMEOUT;
        delay_1ms();
    }
    if (serialBuffer[0] != FPS_ID_STARTCODE_H || serialBuffer[1] != FPS_ID_STARTCODE_L) return FPS_RX_BADPACKET;
    address = ((uint32_t)serialBuffer[2] << 24) | ((uint32_t)serialBuffer[3] << 16) | ((uint32_t)serialBuffer[4] << 8) | ((uint32_t)serialBuffer[5]);
    if (address != stream->deviceAddress) return FPS_RX_WRONG_ADDRESS;
    if (serialBuffer[6] != FPS_ID_DATAPACKET && serialBuffer[6] != FPS_ID_ACKPACKET && serialBuffer[6] != FPS_ID_ENDDATAPACKET) {
        return FPS_RX_WRONG_RESPONSE;
    }

    stream->rxPacketType = serialBuffer[6];
    stream->rxDataBufferLength = serialBuffer[7] << 8 | serialBuffer[8];
    stream->rxDataBufferLength -= 2;
    *receive_length = stream->rxDataBufferLength;

    checksum = serialBuffer[6] + serialBuffer[7] + serialBuffer[8];
    //read data
    if (stream->rxDataBufferLength) {
        time = 0;
        read_bytes = stream->read(receive_buffer, stream->rxDataBufferLength, timeout);
        while (read_bytes < stream->rxDataBufferLength) {
            read_bytes += stream->read(receive_buffer + read_bytes, stream->rxDataBufferLength - read_bytes, 10);
            time += 10;
            if (time == timeout) return FPS_RX_TIMEOUT;
            delay_1ms();
        }
    }
    // read ckecksum
    time = 0;
    read_bytes = stream->read(serialBuffer, 2, 2);
    while (read_bytes < 2) {
        read_bytes += stream->read(serialBuffer + read_bytes, 2 - read_bytes, 1);
        time++;
        if (time == timeout) return FPS_RX_TIMEOUT;
        delay_1ms();
    }
    for (uint16_t i = 0; i < stream->rxDataBufferLength; i++) checksum += receive_buffer[i];
    if (checksum != (serialBuffer[0] << 8 | serialBuffer[1])) return FPS_RX_WRONG_CHECKSUM;
    return FPS_RX_OK;
}
/*
*   @brief: verifyPassword
*   @parameter: pointer to finger print structure
*   @parameter: device password
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t verifyPassword (__FPS *stream, uint32_t inputPassword) {
  uint8_t inputPasswordBytes[4] = {0};  //to store the split password
  inputPasswordBytes[0] = (inputPassword >> 24) & 0xFFU;
  inputPasswordBytes[1] = (inputPassword >> 16) & 0xFFU;
  inputPasswordBytes[2] = (inputPassword >> 8) & 0xFFU;
  inputPasswordBytes[3] = (inputPassword) & 0xFFU;

  sendPacket(stream, FPS_CMD_VERIFYPASSWORD, inputPasswordBytes, 4); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if (response == FPS_RX_OK) { //if the response packet is valid
      if (stream->rxConfirmationCode == FPS_RESP_OK) {
          //save the input password if it is correct
          //this is actually redundant, but can make sure the right password is available to execute further commands
          stream->devicePassword = inputPassword;
          return FPS_RESP_OK; //password is correct
      }
  }
  return response; //return packet receive error code

}
/*
*   @brief: setPassword of fingerprint
*   @parameter: pointer to finger print structure
*   @parameter: desired password 
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t setPassword (__FPS *stream, uint32_t inputPassword) {
    uint8_t inputPasswordBytes[4] = { 0 };  //to store the split password
    inputPasswordBytes[0] = (inputPassword >> 24) & 0xFFU;
    inputPasswordBytes[1] = (inputPassword >> 16) & 0xFFU;
    inputPasswordBytes[2] = (inputPassword >> 8) & 0xFFU;
    inputPasswordBytes[3] = (inputPassword) & 0xFFU;

  sendPacket(stream, FPS_CMD_SETPASSWORD, inputPasswordBytes, 4); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confrim code will be saved when the response is received
      stream->devicePassword = inputPassword; //save the new password (Long)
	  return FPS_RESP_OK; //password setting complete
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief: setAddress
*   @parameter: pointer to finger print structure
*   @parameter: new address for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t setAddress (__FPS *stream ,uint32_t address) {
  uint8_t addressArray[4] = {0}; //just so that we do not need to alter the existing address before successfully changing it
  addressArray[0] = (address >> 24) & 0xFF;
  addressArray[1] = (address >> 16) & 0xFF;
  addressArray[2] = (address >> 8) & 0xFF;
  addressArray[3] = (address) & 0xFF;

  sendPacket(stream, FPS_CMD_SETDEVICEADDRESS, addressArray, 4); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if( stream->rxConfirmationCode == FPS_RESP_OK ) { //the confrim code will be saved when the response is received
        stream->deviceAddress = address; //save the new address
        return FPS_RESP_OK; //address setting complete
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief: setBaudrate
*   @parameter: pointer to finger print structure
*   @parameter: new buadrate for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t setBaudrate (__FPS *stream ,uint32_t baud) {
  uint8_t baudNumber = baud / 9600; //check if the baudrate is a multiple of 9600
  uint8_t dataArray[2] = {0};

  if((baudNumber > 0) && (baudNumber < 13)) { //should be between 1 (9600bps) and 12 (115200bps)
    dataArray[0] = 4;  //the code for the system parameter number, 4 means baudrate
    dataArray[1] = baudNumber; 

    sendPacket(stream, FPS_CMD_SETSYSPARA, dataArray, 2); //send the command and data
    uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

    if(response == FPS_RX_OK) { //if the response packet is valid
      if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
          stream->deinitializePort();
          if (stream->initializePort(stream->deviceBaudrate) == FPS_RESP_OK) {
              stream->deviceBaudrate = baud;
              return FPS_RESP_OK; //baudrate setting complete
        }
      }
    }
   return response; //return packet receive error code
  }
  else {
    return FPS_BAD_VALUE;
  }
}
/*
*   @brief: setSecurityLevel
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t setSecurityLevel (__FPS *stream ,uint8_t level) {
  uint8_t dataArray[2] = {0};

  if((level > 0) && (level < 6)) { //should be between 1 and 5
    dataArray[0] =  5; //the code for the system parameter number, 5 means the security level
    dataArray[1] = level;

    sendPacket(stream , FPS_CMD_SETSYSPARA, dataArray, 2); //send the command and data
    uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

    if(response == FPS_RX_OK) { //if the response packet is valid
      if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
        stream->securityLevel = level;  //save new value
        return FPS_RESP_OK; //security level setting complete
      }
    }
    return response; //return packet receive error code
  }
  else {
    return FPS_BAD_VALUE; //the received parameter is invalid
  }
}
/*
*   @brief: setDataLength
*   @parameter: pointer to finger print structure
*   @parameter: new data length level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t setDataLength (__FPS *stream ,uint16_t length) {
  uint8_t dataArray[2] = {0};

  if((length == 32) || (length == 64) || (length == 128) || (length == 256)) { //should be 32, 64, 128 or 256 bytes
    if(length == 32)
      dataArray[1] = 0;  //low byte
    else if(length == 64)
      dataArray[1] = 1;  //low byte
    else if(length == 128)
      dataArray[1] = 2;  //low byte
    else if(length == 256)
      dataArray[1] = 3;  //low byte

    dataArray[0] = 6; //the code for the system parameter number
    sendPacket(stream, FPS_CMD_SETSYSPARA, dataArray, 2); //send the command and data
    uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

    if(response == FPS_RX_OK) { //if the response packet is valid
      if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
        stream->dataPacketLength = length;  //save the new data length
		return FPS_RESP_OK; //length setting complete
      }
    }
    return response; //return packet receive error code
  }
  else {
    return FPS_BAD_VALUE; //the received parameter is invalid
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t portControl (__FPS *stream ,uint8_t value) {
  uint8_t dataArray[1] = {0};

  if((value == 0) || (value == 1)) { //should be either 1 or 0
    dataArray[0] = value;
    sendPacket(stream, FPS_CMD_PORTCONTROL, dataArray, 1); //send the command and data
    uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

    if(response == FPS_RX_OK) { //if the response packet is valid
      if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
        return FPS_RESP_OK; //port setting complete
      }
    }
    return response; //return packet receive error code
  }
  else {
    return FPS_BAD_VALUE; //the received parameter is invalid
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t readSysPara(__FPS *stream) {
  sendPacket(stream, FPS_CMD_READALL_SYSPARA, NULL , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response
  uint16_t len, index = 0;
  uint8_t data_buffer[512];
  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
        while (receiveDataPacket(stream, data_buffer + index, &len, FPS_DEFAULT_TIMEOUT) == FPS_RX_OK && stream->rxPacketType == FPS_ID_DATAPACKET) {
            index += len;
        }
        if (stream->rxPacketType == FPS_ID_ENDDATAPACKET) {
            stream->templateCount = ((uint16_t)(data_buffer[4]) << 8) + data_buffer[5];
            stream->securityLevel = ((uint16_t)(data_buffer[6]) << 8) + data_buffer[7];

            stream->deviceAddress = ((uint32_t)(data_buffer[8]) << 24) + ((uint32_t)(data_buffer[9]) << 16) + ((uint32_t)(data_buffer[10]) << 8) + data_buffer[11];

            stream->dataPacketLengthCode = ((uint16_t)(data_buffer[12]) << 8) + data_buffer[13];
            stream->baudMultiplier = ((uint16_t)(data_buffer[14]) << 8) + data_buffer[15];
            memcpy(stream->deviceName, &data_buffer[28], 32);

            if (stream->dataPacketLengthCode == 0)
                stream->dataPacketLength = 32;
            else if (stream->dataPacketLengthCode == 1)
                stream->dataPacketLength = 64;
            else if (stream->dataPacketLengthCode == 2)
                stream->dataPacketLength = 128;
            else if (stream->dataPacketLengthCode == 3)
                stream->dataPacketLength = 256;

            stream->deviceBaudrate = (uint32_t)(stream->baudMultiplier * 9600);  //baudrate is retrieved as a multiplier
            return FPS_RESP_OK; //just the confirmation code only
        }
        else {
            return FPS_RESP_RECIEVEERR;
        }
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t getTemplateCount(__FPS *stream) {
  sendPacket(stream, FPS_CMD_TEMPLATECOUNT, 0 , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      stream->templateCount = ((uint16_t)(stream->rxDataBuffer[1]) << 8) + stream->rxDataBuffer[0];  //high byte + low byte
      return FPS_RESP_OK;
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t captureAndRangeSearch (__FPS *stream ,uint16_t captureTimeout, uint16_t startLocation, uint16_t count) {
  if(captureTimeout > 25500) { //25500 is the max timeout the device supports
    return FPS_BAD_VALUE;
  }

  if( startLocation > stream->templateCount ) { //if not in range (0-999)
    return FPS_BAD_VALUE;
  }

  if((startLocation + count) > 1001) { //if range overflows
    return FPS_BAD_VALUE;
  }

  uint8_t dataArray[5] = {0}; //need 5 bytes here

  //generate the data array
  dataArray[4] = (uint8_t)(captureTimeout / 140);  //this byte is sent first
  dataArray[3] = ((startLocation-1) >> 8) & 0xFFU;  //high byte
  dataArray[2] = (uint8_t)((startLocation-1) & 0xFFU);  //low byte
  dataArray[1] = (count >> 8) & 0xFFU; //high byte
  dataArray[0] = (uint8_t)(count & 0xFFU); //low byte

  sendPacket(stream, FPS_CMD_SCANANDRANGESEARCH, dataArray, 5); //send the command, there's no additional data
  uint8_t response = receivePacket(stream ,captureTimeout + 100); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      stream->fingerId = ((uint16_t)(stream->rxDataBuffer[3]) << 8) + stream->rxDataBuffer[2];  //high byte + low byte

      stream->matchScore = ((uint16_t)(stream->rxDataBuffer[1]) << 8) + stream->rxDataBuffer[0];  //data length will be 4 here
      return FPS_RESP_OK;
    }
    else {
    	stream->fingerId = 0;
    	stream->matchScore = 0;
	  return stream->rxConfirmationCode;  //setting was unsuccessful and so send confirmation code
    }
  }
  else {
    return response; //return packet receive error code
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t captureAndFullSearch (__FPS *stream) {
  sendPacket(stream , FPS_CMD_SCANANDFULLSEARCH, 0 , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , 3000); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      stream->fingerId = ((uint16_t)(stream->rxDataBuffer[3]) << 8) + stream->rxDataBuffer[2];  //high byte + low byte
      stream->fingerId += 1;  //because IDs start from #1
      stream->matchScore = ((uint16_t)(stream->rxDataBuffer[1]) << 8) + stream->rxDataBuffer[0];  //data length will be 4 here
	  return FPS_RESP_OK;
    }
    else {
      stream->fingerId = 0;
      stream->matchScore = 0;
	  return stream->rxConfirmationCode;  //setting was unsuccessful and so send confirmation code
    }
  }
  else {
    return response; //return packet receive error code
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t generateImage (__FPS *stream) {
  sendPacket(stream , FPS_CMD_SCANFINGER, 0 , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t exportImage (__FPS *stream) {
  sendPacket(stream , FPS_CMD_EXPORTIMAGE, 0 , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
    else {
      return stream->rxConfirmationCode;  //setting was unsuccessful and so send confirmation code
    }
  }
  else {
    return response; //return packet receive error code
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t importImage (__FPS *stream ,uint8_t* dataBuffer) {
  sendPacket(stream, FPS_CMD_IMPORTIMAGE, dataBuffer, 64); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
    else {
      return stream->rxConfirmationCode;  //setting was unsuccessful and so send confirmation code
    }
  }
  else {
    return response; //return packet receive error code
  }
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: select bufferID 1 or 2
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t generateCharacter (__FPS *stream ,uint8_t bufferId) {
  if(bufferId != 1 && bufferId != 2) { //if the value is not 1 or 2
	return FPS_BAD_VALUE;
  }
  uint8_t dataBuffer[1] = {bufferId}; //create data array
  
  sendPacket(stream , FPS_CMD_IMAGETOCHARACTER, dataBuffer, 1);
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: none
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t generateTemplate (__FPS *stream) {
  sendPacket(stream , FPS_CMD_GENERATETEMPLATE, 0 , 0); //send the command, there's no additional data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: select bufferID 1 or 2
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t exportCharacter (__FPS *stream ,uint8_t bufferId) {
  uint8_t dataBuffer[1] = {bufferId}; //create data array
  sendPacket(stream , FPS_CMD_IMPORTIMAGE, 0 , 0);
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: select bufferID 1 or 2
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t importCharacter (__FPS *stream ,uint8_t bufferId, uint8_t* dataBuffer) {
  uint8_t dataArray[sizeof(dataBuffer)+1] = {0}; //create data array
  dataArray[sizeof(dataBuffer)];
  sendPacket(stream , FPS_CMD_IMPORTIMAGE, 0 , 0);
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: select bufferID 1 or 2
*   @parameter: pasge ID in internal flash
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t saveTemplate (__FPS *stream ,uint8_t bufferId, uint16_t location) {
  if(!((bufferId > 0) && (bufferId < 3))) { //if the value is not 1 or 2
    return FPS_BAD_VALUE;
  }

  if((location > stream->templateCount) || (location < 1)) { //if the value is not in range
	return FPS_BAD_VALUE;
  }
  uint8_t dataArray[3] = {0}; //create data array
  dataArray[0] = bufferId;  //highest byte
  dataArray[1] = (location >> 8) & 0xFFU; //high byte of location
  dataArray[2] = (location & 0xFFU); //low byte of location

  sendPacket(stream, FPS_CMD_STORETEMPLATE, dataArray, 3); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t loadTemplate (__FPS *stream ,uint8_t bufferId, uint16_t location) {
  if(!((bufferId > 0) && (bufferId < 3))) { //if the value is not 1 or 2
	return FPS_BAD_VALUE;
  }

  if((location > stream->templateCount) || (location < 1)) { //if the value is not in range
    return FPS_BAD_VALUE;
  }

  uint8_t dataArray[3] = {0}; //create data array
  dataArray[0] = bufferId;  //highest byte
  dataArray[1] = (location >> 8) & 0xFFU; //high byte of location
  dataArray[2] = (location & 0xFFU); //low byte of location

  sendPacket(stream, FPS_CMD_LOADTEMPLATE, dataArray, 3); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t deleteTemplate (__FPS *stream ,uint16_t startLocation, uint16_t count) {
  if((startLocation > stream->templateCount) || (startLocation < 1)) { //if the value is not 1 or 2
    return FPS_BAD_VALUE;
  }

  if((count + startLocation) > stream->templateCount + 1) { //if the value is not in range
    return FPS_BAD_VALUE;
  }

  uint8_t dataArray[4] = {0}; //create data array
  dataArray[0] = (startLocation >> 8) & 0xFFU; //high byte of location
  dataArray[1] = (startLocation & 0xFFU); //low byte of location
  dataArray[2] = (count >> 8) & 0xFFU; //high byte of total no. of templates to delete
  dataArray[3] = (count & 0xFFU); //low byte of count
  
  sendPacket(stream, FPS_CMD_DELETETEMPLATE, dataArray, 4); //send the command and data
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: none
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t clearLibrary (__FPS *stream) {

  sendPacket(stream , FPS_CMD_CLEARLIBRARY, 0 , 0); //send the command
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t matchTemplates (__FPS *stream) {

  sendPacket(stream , FPS_CMD_MATCHTEMPLATES, 0 , 0); //send the command
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response
  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      stream->matchScore = (uint16_t)(stream->rxDataBuffer[1] << 8) + stream->rxDataBuffer[0];
      return FPS_RESP_OK; //just the confirmation code only
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t searchLibrary (__FPS *stream ,uint8_t bufferId, uint16_t startLocation, uint16_t count) {
  if(bufferId != 1 && bufferId != 2) { //if the value is not 1 or 2
    return FPS_BAD_VALUE;
  }

  if((startLocation > stream->templateCount) ) { //if not in range (0-999)
    return FPS_BAD_VALUE;
  }

  if((startLocation + count) > stream->templateCount) { //if range overflows
    return FPS_BAD_VALUE;
  }

  uint8_t dataArray[5] = {0};
  dataArray[0] = bufferId;
  dataArray[1] = (startLocation >> 8) & 0xFFU;  //high byte
  dataArray[2] = (startLocation & 0xFFU); //low byte
  dataArray[3] = (count >> 8) & 0xFFU; //high byte
  dataArray[4] = (count & 0xFFU); //low byte

  sendPacket(stream , FPS_CMD_HISPEEDSEARCH, dataArray, 5); //send the command
  uint8_t response = receivePacket(stream , FPS_DEFAULT_TIMEOUT); //read response

  if(response == FPS_RX_OK) { //if the response packet is valid
    if(stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
      stream->fingerId = ((uint16_t)(stream->rxDataBuffer[0]) << 8) + stream->rxDataBuffer[1];  //add high byte and low byte
      stream->matchScore = ((uint16_t)(stream->rxDataBuffer[2]) << 8) + stream->rxDataBuffer[3];  //add high byte and low byte

      return FPS_RESP_OK; //just the confirmation code only
    }
    else {
      //fingerId = 0 doesn't mean the match was found at location 0
      //instead it means an error. check the confirmation code to determine the problem
      stream->fingerId = 0;
      stream->matchScore = 0;
    }
  }
  return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: pointer to a buffer containing image at least (288 * 256  / 2) bytes ~ 36KB
*   @parameter: none
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t getImage(__FPS* stream,uint8_t* image_buffer) {

    sendPacket(stream, FPS_CMD_EXPORTIMAGE, 0, 0); //send the command
    uint8_t response = receivePacket(stream, FPS_DEFAULT_TIMEOUT); //read response
    uint16_t len , index = 0;
    if (response == FPS_RX_OK) { //if the response packet is valid
        if (stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
            while (receiveDataPacket(stream, image_buffer + index, &len, FPS_DEFAULT_TIMEOUT) == FPS_RX_OK && stream->rxPacketType == FPS_ID_DATAPACKET) {
                index += len;
            }
            if (stream->rxPacketType == FPS_ID_ENDDATAPACKET) {
                return FPS_RESP_OK; //just the confirmation code only
            }
            else {
                return FPS_RESP_RECIEVEERR;
            }
        }
    }
    return response; //return packet receive error code
}
/*
*   @brief:
*   @parameter: pointer to finger print structure
*   @parameter: new security level for device
*   @return: on success FPS_RESP_OK or 0 , packet information is in the stream structure
*
*/
uint8_t generateRandomNumber(__FPS* stream,uint32_t *random) {
    sendPacket(stream, FPS_CMD_GETRANDOMCODE, 0, 0); //send the command and data
    uint8_t response = receivePacket(stream, FPS_DEFAULT_TIMEOUT); //read response

    if (response == FPS_RX_OK) { //if the response packet is valid
        if (stream->rxConfirmationCode == FPS_RESP_OK) { //the confirm code will be saved when the response is received
            *random = (uint32_t)stream->rxDataBuffer[0] | ((uint32_t)stream->rxDataBuffer[1] << 8) | ((uint32_t)stream->rxDataBuffer[2] << 16) | ((uint32_t)stream->rxDataBuffer[3] << 24);
            return FPS_RESP_OK; //port setting complete
        }
    }
    return response; //return packet receive error code
}

/********************************END OF FILE*****************************************************/
