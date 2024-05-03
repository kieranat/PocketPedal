#include <Arduino.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define constants and variables
byte address = 0x11;
int CS1 = 15;  // CS pin for the first digital potentiometer
int CS2 = 32;  // CS pin for the second digital potentiometer
int CS3 = 33;  // CS pin for the third digital potentiometer

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX characteristic UUID for WRITE
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX characteristic UUID for NOTIFY

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

void digitalPotWrite(int value) {
  // Function to write a value to multiple digital potentiometers
  int csPins[] = {CS1, CS2, CS3};
  for (int csPin : csPins) {
    digitalWrite(csPin, LOW);
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(csPin, HIGH);
  }
  Serial.print("Written value to all potentiometers: ");
  Serial.println(value);
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        int receivedInt = 0;  // to store the converted integer
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]); // Print each character in the received string
          receivedInt = receivedInt * 10 + (rxValue[i] - '0');  // Convert string to integer assuming ASCII digits
        }
        Serial.println();
        Serial.print("Converted integer: ");
        Serial.println(receivedInt);
        Serial.println("*********");
        if (receivedInt >= 0 && receivedInt <= 255) {
          digitalPotWrite(receivedInt);  // Update the potentiometer value
        } else {
          Serial.println("Received integer out of range (0-255).");
        }
      } else {
        Serial.println("Received BLE write with empty payload.");
      }
    }
};

void setup() {
  Serial.begin(9600);
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  pinMode(CS3, OUTPUT);
  SPI.begin();  // Initializes SPI communication

  BLEDevice::init("LED Controller");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                     );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                        );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
