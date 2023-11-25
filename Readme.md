# R30x Fingerprint Library
### C library to communicate with R30x Fingerprint module
**This Library is tested with R308 module on stm32H7 series.**

This library is written in `C` language and compatible with most platforms to use. It is not dependent on any library or SDK. A data structure is defined that handles all the communications. The user must provide the library with the Serial communication function.
At the beginning of your code make an instance of fingerprint structure like shown below.
```C
__FPS finger = { 0 };
```
The structure contains 4 function pointers for initialization, read, write and deinitilization of the Serial communication.
You should implement all the necessary code for communications in 4 functions with the prototype in the code below:
```C
uint8_t initilize(uint32_t baud){
  return 0; // returns 0 on success
}
uint8_t DeinitializeSerialPort(void){
  return 0; // returns 0 on success
}
uint32_t writeSerialBuffer(unsigned char* buff, U32 numBytestoWrite, U16 timout){
  uint32_t bytes_have_written = 0;

   return bytes_have_written; // returns number of bytes has written
}
uint32_t readSerialBuffer(unsigned char* buff, U32 numBytestoRead, U16 timeout){
  uint32_t bytes_have_read = 0;

   return ibytes_have_read; // returns number of bytes has read
}
```
Then you need to set the function pointers to the implemented functions:
```C
  finger.read = readSerialBuffer;
  finger.write = writeSerialBuffer;
  finger.initializePort = initilize;
  finger.deinitializePort = DeinitializeSerialPort;
  if (R30X_init(&finger, FPS_DEFAULT_PASSWORD, FPS_DEFAULT_ADDRESS) == FPS_RESP_OK) {
    //successful initilization of fingerprint sensor
    // the default baudrate of sensor is 57600 bps , No parity and one stop bit
    //
  }
```
After the initilization of module you can enroll a finger like the code below. For each finger enrolment, the module should scan that finger 2 times, then compares the images and if they fit together it will produce a template that can be stored in the fingerprint template library.
```C
int number_retries = 20;
int ID = 1;
for (int i = 0; i < number_retries; i++) {
      if (generateImage(&finger) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
        if (generateCharacter(&finger, 1) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
          //Image taken successfully. remove your finger
          break;
        }
      }
      if (i == number_retries) {
        //No finger detected. Enrol Canceled!"
        return;
      }
      Delay_ms(100); // implement you delay function
    }
    // remove ypur finger from sensor
    // Put your finger on the sensor aganin for the second scan
    for (int i = 0; i < number_retries; i++) {
      if (generateImage(&finger) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
        if (generateCharacter(&finger, 2) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
          //Image taken successfully. remove your finger
          break;
        }
      }
      if (i == number_retries) {
        //No finger detected. Enrol Canceled!
        return;
      }
      Delay_ms(100); // implement you delay function
    }
    if (generateTemplate(&finger) == FPS_RX_OK) {
      if (finger.rxConfirmationCode == FPS_RESP_OK) {
        //Model generated successfully
      }
      else {
        // Fingers did not match.
        return;
      }
    }
    if (saveTemplate(&finger, 1, ID) == FPS_RX_OK) {
      if (finger.rxConfirmationCode == FPS_RESP_OK) {
        //Model Stored successfully at page number (ID)
        return;
      }
      else {
        //Problem Storing model");
        return;
      }
    }
```
After enrolling a fingerprint you can search for the scanned finger like:
```C
//Put your finger on the sensor
int number_retries = 20;
    for (int i = 0; i < number_retries; i++) {
      if (generateImage(&finger) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
        if (generateCharacter(&finger, 1) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
          if (searchLibrary(&finger, 1, 0, 512) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
            //Finger detected 
            ID = finger.fingerId;
            score = finger.matchScore;
            //its better to implement this code in a function and return from here
            return;
          }
        }
        break;
      }
      Delay_ms(100); // implement you delay function
    }
    //loop ended and No finger detected
```
You can also download the fingerprint image from the module. The picture is 256 * 288 pixels and 8-bit grayscale. Connecting via UAR communication the module only sends the upper 4bits of each pixel and combines two adjacent pixels data into one bytes. So the image that you get via UART is 36 KBytes in size and you should have enough space to receive the image data.
```C
//Put your finger on the sensor
    uint8_t finger_picture[256 * 288] = { 0 };
    int number_retries = 20;
    for (int i = 0; i < number_retries; i++) {
      if (generateImage(&finger) == FPS_RX_OK && finger.rxConfirmationCode == FPS_RESP_OK) {
        //Picture Captured.Downloadind image...
        if (getImage(&finger, finger_picture) == FPS_RX_OK) {
          //Picture Downloaded successfully.

          //decoposing two adjacent pixels information and converting image from 4bit to 8bit per pixel.
          for (int index = (256 * 288) - 1; index > 0; index -= 2) {
            finger_picture[index] = (finger_picture[index / 2] << 4);
            finger_picture[index - 1] = (finger_picture[index / 2] & 0xf0);
          }
          return;
        }
        else {
          //Problem downloading image
          return;
        }
        break;
      }
      Delay_ms(100); // implement you delay function
    }
    //No finger detected
```