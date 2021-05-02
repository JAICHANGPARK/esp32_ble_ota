
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

BLECharacteristic * pOtaControlCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

#define SERVICE_UUID                    "0000FF00-0000-1000-8000-00805f9b34fb"  // UART service UUID
#define CHARACTERISTIC_UUID_RX          "0000FF01-0000-1000-8000-00805f9b34fb"  // 데이터 쓰는것
#define CHARACTERISTIC_UUID_TX          "0000FF02-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_OTA_CONTROL "0000FF03-0000-1000-8000-00805f9b34fb"  // 총 버퍼 컨트롤용

uint8_t bleFileBuff[1024] = {0x00,};
bool isReceived = false;
bool receivedIndicator = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println(rxValue.length());
        //        Serial.println("*********");
        //        Serial.print("Received Value: ");
        isReceived = true;
        for (int i = 0; i < rxValue.length(); i++)
        {
          bleFileBuff[i] = (uint8_t)rxValue[i];
          //          Serial.print(rxValue[i]);
        }

        //        Serial.println();
        //        Serial.println("*********");
      }
    }
};

class OtaControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
       
        isReceived = true;
        for (int i = 0; i < rxValue.length(); i++)
        {
         
        }
      }
    }
};


void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);


  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pOtaControlCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_OTA_CONTROL,
      BLECharacteristic::PROPERTY_WRITE
   );
   pOtaControlCharacteristic->setCallbacks(new OtaControlCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (deviceConnected) {
    //        pTxCharacteristic->setValue(&txValue, 1);
    //        pTxCharacteristic->notify();
    //        txValue++;
    if (isReceived) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < 512; i++) {
        Serial.print(bleFileBuff[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.println("*********");
      receivedIndicator = !receivedIndicator;
      digitalWrite(5, receivedIndicator);
      
      isReceived = false;
    }

    delay(10); // bluetooth stack will go into congestion, if too many packets are sent

  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
