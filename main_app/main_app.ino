
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
int received_counter = 0;
bool receivedIndicator = false;
bool isPsramSetting = false;
int n_elements = 0;
int n_chunk_length = 0;

uint8_t *int_array ;
int buff_counter = 0;
int chunk_counter = 0;

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
        
        for (int i = 0; i < rxValue.length(); i++)
        {
          bleFileBuff[i] = (uint8_t)rxValue[i];
          //          Serial.print(rxValue[i]);
          received_counter = received_counter + 1;
        }
         Serial.print("received_counter: ");
         Serial.println(received_counter);
        if(received_counter == 512){
          Serial.println("received_counter is 512");
          isReceived = true;
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
             Serial.println("OtaControlCallbacks *********");
        for (int i = 0; i < rxValue.length(); i++)
        {
                   Serial.print(rxValue[i]);
        }

      
        n_elements = ((uint8_t)rxValue[0] << 24) & 0xFF000000
        | ((uint8_t)rxValue[1] << 16) & 0x00FF0000
        | ((uint8_t)rxValue[2] << 8) & 0x0000FF00
        | ((uint8_t)rxValue[3]) & 0x000000FF;

        n_chunk_length = ((uint8_t)rxValue[4] << 24) & 0xFF000000
        | ((uint8_t)rxValue[5] << 16) & 0x00FF0000
        | ((uint8_t)rxValue[6] << 8) & 0x0000FF00
        | ((uint8_t)rxValue[7]) & 0x000000FF;

        Serial.println();
          isPsramSetting = true;
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
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
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
      if(received_counter == 512){
        for (int i = 0; i < 512; i++) {
        Serial.print(bleFileBuff[i], HEX);
        Serial.print(" ");
        int_array[buff_counter] = bleFileBuff[i];
        buff_counter++;
        }
      
      
     
        Serial.println();
        Serial.print("buff_counter: ");
        Serial.print(buff_counter);
        Serial.println(" *********");
        receivedIndicator = !receivedIndicator;
        digitalWrite(5, receivedIndicator); 
        pTxCharacteristic->setValue(chunk_counter);
        pTxCharacteristic->notify();

        chunk_counter++;
      
        if(buff_counter == n_elements){
          Serial.println(">>>>>>>>>>>>> Same? >>>>>>>>>>>>>>>>>>>>");
        }
        received_counter = 0;
      }
       isReceived = false;
    }
   

    if(isPsramSetting){
      Serial.print("n_elements: ");
      Serial.println(n_elements);
      int temp = n_elements + 1024;
      if(n_elements != 0){
        //psram setting
       int_array = (uint8_t *) ps_malloc(temp * sizeof(uint8_t));
      
       Serial.println("Set PSRAM ps_malloc");
      
      }else{
       //psram free 
       free(int_array);
        Serial.println("free PSRAM ");
      
      }
      isPsramSetting = false;
    }

    delay(10); // bluetooth stack will go into congestion, if too many packets are sent

  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
     free(int_array);
      n_elements = 0;
      for(int i = 0; i < 1024; i++){
        bleFileBuff[i] = 0x00;
      }
    
      isReceived = false;
      receivedIndicator = false;
      isPsramSetting = false;
      n_elements = 0;
      n_chunk_length = 0;


      buff_counter = 0;
      chunk_counter = 0;
    }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
