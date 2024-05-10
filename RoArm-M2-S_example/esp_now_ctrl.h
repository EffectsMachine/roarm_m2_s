const int MAX_FOLLOWERS = 10;  // Maximum number of follower devices

typedef struct struct_message {
  byte devCode;
  float base;
  float shoulder;
  float elbow;
  float hand;
  byte cmd;
  char message[210];
} struct_message;

struct_message espNowMessage;
struct_message espNowMegsRecv;
esp_now_peer_info_t peerInfo;

#define ESP_NOW_CHANNEL 0
#define ESP_NOW_ENCRYPT false

uint8_t thisDevMac[6];
uint8_t singleFollowerDev[6];


// input a mac[6]:{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
// return String: "FF:FF:FF:FF:FF:FF"
String macToString(uint8_t mac[6]) {
  char macStr[18]; // 6 pairs of 2 characters + null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}


void getThisDevMacAddress() {
  WiFi.macAddress(thisDevMac);

  String getInfoJsonString = macToString(thisDevMac);

  jsonInfoHttp.clear();
  jsonInfoHttp["mac"] = getInfoJsonString;

  Serial.println(getInfoJsonString);
}


void changeEspNowMode(byte inputMode) {
  switch(inputMode) {
  case 0: espNowMode = inputMode;
    if (InfoPrint == 1) {Serial.println("esp-now mode: none");}
    screenLine_3 = "ESP-NOW: NONE";
    oled_update();

    jsonInfoHttp.clear();
    jsonInfoHttp["info"] = "esp-now mode: none";

    break;
  case 1: espNowMode = inputMode;
    espNowMessage.cmd = 0;
    if (InfoPrint == 1) {Serial.println("esp-now mode: flow-leader(group)");}
    screenLine_3 = "ESP-NOW: F-LEADER-B";
    oled_update();

    jsonInfoHttp.clear();
    jsonInfoHttp["info"] = "esp-now mode: flow-leader(group)";

    break;
  case 2: espNowMode = inputMode;
    espNowMessage.cmd = 0;
    if (InfoPrint == 1) {Serial.println("esp-now mode: flow-leader(single)");}
    screenLine_3 = "ESP-NOW: F-LEADER-S";
    oled_update();

    jsonInfoHttp.clear();
    jsonInfoHttp["info"] = "esp-now mode: flow-leader(single)";

    break;
  case 3: espNowMode = inputMode;
    if (InfoPrint == 1) {Serial.println("esp-now mode: follower");}
    screenLine_3 = "ESP-NOW: > FOLLOWER <";
    oled_update();

    jsonInfoHttp.clear();
    jsonInfoHttp["info"] = "esp-now mode: follower";

    break;
  }
}


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void macStringToByteArray(const String& macString, uint8_t* byteArray) {
  for (int i = 0; i < 6; i++) {
    byteArray[i] = strtol(macString.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  }
  return;
}


void OnDataRecv(const unsigned char* mac, const unsigned char* incomingData, int len) {
  if (espNowMode != 3){
    return;
  }

  if (!ctrlByBroadcast) {
    if (memcmp(mac, mac_whitelist_broadcast, 6) != 0) {
      return;
    }
  }

  memcpy(&espNowMegsRecv, incomingData, sizeof(espNowMegsRecv));
  if (InfoPrint == 1) {
    Serial.print("Bytes received: ");Serial.println(len);
  }

  DeserializationError err = deserializeJson(jsonCmdReceive, espNowMegsRecv.message);
  switch(espNowMegsRecv.cmd) {
  case 0:
    RoArmM2_allJointAbsCtrl(espNowMegsRecv.base,
                            espNowMegsRecv.shoulder,
                            espNowMegsRecv.elbow,
                            espNowMegsRecv.hand,
                            0,
                            0);break;
  case 1:
    if (err == DeserializationError::Ok) {
      jsonCmdReceiveHandler();
    };break;
  case 2:
    if (err == DeserializationError::Ok) {
      runNewJsonCmd = true;
    };break;
  case 3:
    Serial.println(espNowMegsRecv.message);
  }
}


void initEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // register esp-now sending call back function.
  esp_now_register_send_cb(OnDataSent);

  // register esp-now receving call back function.
  esp_now_register_recv_cb(OnDataRecv);

  // register peer
  peerInfo.channel = ESP_NOW_CHANNEL;  
  peerInfo.encrypt = ESP_NOW_ENCRYPT;
}


void registerNewFollowerToPeer(String inputMac) {
  if (inputMac.length() != 17) {
    Serial.println("invalid MAC address format.");
    return;
  }

  uint8_t macArray[6];
  macStringToByteArray(inputMac, macArray);
  for (int i = 0; i < 6; i++) {
    singleFollowerDev[i] = macArray[i];
  }
  memcpy(peerInfo.peer_addr, macArray, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer: " + inputMac);

    jsonInfoHttp.clear();
    jsonInfoHttp["info"] = "Failed to add peer";
    jsonInfoHttp["mac"] = inputMac;

    return;
  }
  Serial.println("add peer: " + inputMac);

  jsonInfoHttp.clear();
  jsonInfoHttp["info"] = "add peer";
  jsonInfoHttp["mac"]  = inputMac;
}


void deleteFollower(String inputMac) {
  if (inputMac.length() != 17) {
    Serial.println("invalid MAC address format.");
    return;
  }

  uint8_t macArray[6];
  macStringToByteArray(inputMac, macArray);
  esp_now_del_peer(macArray);

  jsonInfoHttp.clear();
  jsonInfoHttp["info"] = "delete peer";
  jsonInfoHttp["mac"]  = inputMac;

  if (InfoPrint == 1) {
    Serial.println("delete peer: " + inputMac);
  }
}


void espNowGroupSend(byte devCodeIn, float bIn, float sIn, float eIn, float hIn, byte cmdIn, String messageIn) {
  espNowMessage.devCode  = devCodeIn;
  espNowMessage.base     = bIn;
  espNowMessage.shoulder = sIn;
  espNowMessage.elbow    = eIn;
  espNowMessage.hand     = hIn;
  espNowMessage.cmd      = cmdIn;
  // espNowMessage.message  = messageIn;
  strcpy(espNowMessage.message, messageIn.c_str());

  esp_err_t result = esp_now_send(0, (uint8_t *) &espNowMessage, sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}


void espNowSingleDevSend(String inputMac, byte devCodeIn, float bIn, float sIn, float eIn, float hIn, byte cmdIn, String messageIn){
  if (inputMac.length() != 17) {
    Serial.println("invalid MAC address format.");
    return;
  }

  espNowMessage.devCode  = devCodeIn;
  espNowMessage.base     = bIn;
  espNowMessage.shoulder = sIn;
  espNowMessage.elbow    = eIn;
  espNowMessage.hand     = hIn;
  espNowMessage.cmd      = cmdIn;
  // espNowMessage.message  = messageIn;
  strcpy(espNowMessage.message, messageIn.c_str());

  uint8_t macArray[6];
  macStringToByteArray(inputMac, macArray);
  for (int i = 0; i < 6; i++) {
    singleFollowerDev[i] = macArray[i];
  }
  esp_err_t result = esp_now_send(
    macArray, 
    (uint8_t *) &espNowMessage,
    sizeof(struct_message));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}


void espNowSingleDevFlowCtrl() {
  espNowMessage.base     = radB;
  espNowMessage.shoulder = radS;
  espNowMessage.elbow    = radE;
  espNowMessage.hand     = radG;

  esp_err_t result = esp_now_send(singleFollowerDev, 
                                  (uint8_t *) &espNowMessage,
                                  sizeof(struct_message));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}


void espNowGroupDevsFlowCtrl() {
  espNowMessage.base     = radB;
  espNowMessage.shoulder = radS;
  espNowMessage.elbow    = radE;
  espNowMessage.hand     = radG;

  esp_err_t result = esp_now_send(0, (uint8_t *) &espNowMessage, sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}


void changeBroadcastMode(bool inputMode, String inputMac) {
  ctrlByBroadcast = inputMode;

  jsonInfoHttp.clear();
  jsonInfoHttp["mode"]  = inputMode;

  uint8_t macArray[6];
  macStringToByteArray(inputMac, macArray);
  for (int i = 0; i < 6; i++) {
    mac_whitelist_broadcast[i] = macArray[i];
  }

  if (InfoPrint == 1) {
    if (ctrlByBroadcast) {
      Serial.println("it can be ctrl by esp-now broadcast cmd.");
      jsonInfoHttp["info"] = "it can be ctrl by esp-now broadcast cmd.";
      jsonInfoHttp["leader mac"] = inputMac;
    } else {
      Serial.println("it won't be ctrl by esp-now broadcast cmd.");
      jsonInfoHttp["info"] = "it won't be ctrl by esp-now broadcast cmd, leader mac: "+inputMac;
    }
  }
}