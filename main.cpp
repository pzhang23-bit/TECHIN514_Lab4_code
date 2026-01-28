#include <Arduino.h> 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
float duration = 0;      // ms duration of echo pulse
float distance = 0;      // raw distance in cm
float filteredDistance = 0; // filtered distance in cm
#define SOUND_SPEED 0.0343 
const int TRIG_PIN = D2; 
const int ECHO_PIN = D3;
#define SOUND_SPEED 0.0343

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "b2f90fcf-4d6a-46e2-8a21-87011eb8be22" // shared service UUID
#define CHARACTERISTIC_UUID "c142758b-8783-46d0-8bbd-231bdd9934d4" // shared characteristic UUID

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here
float lowPassFilter(float input, float output) {
    float alpha = 0.5; // filter coefficient (0 < alpha < 1)
    return (alpha * input) + (1.0 - alpha) * output;
}

void setup() {
    Serial.begin(115200);
    while(!Serial) {
        delay(10); 
    }

    delay(1000); // ensure stable serial connection
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    pinMode(TRIG_PIN, OUTPUT); // trigger pin
    pinMode(ECHO_PIN, INPUT);  // receive echo pin


    // TODO: name your device to avoid conflictions
    BLEDevice::init("YewenZhouESP");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * SOUND_SPEED / 2;
    float rawDistance = duration * 0.034 / 2;

    // 2. clamp the distance to a valid range
    // no skipping here, just clamp invalid values
    float validDistance = rawDistance;
    if (validDistance <= 0 || validDistance > 400.0) {
        validDistance = 400.0; // set as 400 whenno object detected within range
    }

    // TODO: use your defined DSP algorithm to process the readings
   if (distance > 0 && distance < 400) {
        filteredDistance = lowPassFilter(distance, filteredDistance);
    }

    // Print out the raw and processed readings for debugging
    Serial.print("Raw: ");
    Serial.print(distance);
    Serial.print("cm | Filtered: ");
    Serial.print(filteredDistance);
    Serial.println("cm");
    
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        if (filteredDistance < 30.0) {
            unsigned long currentMillis = millis();
            if (currentMillis - previousMillis >= interval) {    
                previousMillis = currentMillis;   
            
                // construct the data string to send
                String displayData = "Raw " + String(distance) + " Dist: " + String(filteredDistance, 1) + "cm";
            
                pCharacteristic->setValue(displayData.c_str());
                pCharacteristic->notify(); 
            
                Serial.print("Raw: ");
                Serial.print(distance);
                Serial.print("cm | Filtered: ");
                Serial.println(displayData);
                Serial.println(">>> BLE Sent: Distance < 30cm!"); 
            }
        }else{
            Serial.println("Distance >= 30cm, not sending data.");
        }
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(100);
}
