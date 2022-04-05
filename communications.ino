#define DEFAULT_CHANNEL 2

// Important: this needs to be allocated outside functions.
esp_now_peer_info_t slave;

uint8_t macSlaves[][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

uint8_t channel = DEFAULT_CHANNEL;
uint8_t id = 0;

command_struct cmd;
command_struct incomingCommand;

// Initialise ESP Now
void initEspNow() {

  WiFi.mode(WIFI_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  int slavesCount = sizeof(macSlaves) / 6 / sizeof(uint8_t);
  for (int i = 0; i < slavesCount; i++) {
    slave.channel = channel;
    slave.encrypt = 0;
    memcpy(slave.peer_addr, macSlaves[i], sizeof(macSlaves[i]));
    esp_now_add_peer(&slave);
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);
}

void sendButtonPress() {
  cmd.cmd = 65;
  cmd.sendHue = 0;
  sendMessage(cmd);
}

void sendColour() {
  uint16_t currentPos = getUserHue();
  cmd.cmd = 66;
  cmd.sendHue = currentPos;
  sendMessage(cmd);
}

void sendMessage(command_struct cmd) {
  uint8_t macAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_err_t result = esp_now_send(macAddr, (uint8_t *) &cmd, sizeof(cmd));

  if (result == ESP_OK) {
    Serial.println("Sent message");
  }
  else {
    Serial.println("Error sending data");
  }
}

// On data received
void onDataReceive(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  memcpy(&incomingCommand, incomingData, sizeof(incomingCommand));
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  memcpy(&incomingCommand, incomingData, sizeof(incomingData));
  Serial.print("Received from: ");
  Serial.println(macStr);
  Serial.print("Bytes received: ");
  Serial.println(len);

  if (incomingCommand.cmd == 65) {
    Serial.println("TEST");
    blinkDevice();
  } else if (incomingCommand.cmd == 66) {
    Serial.print("LIGHTTOUCH ");
    Serial.println(incomingCommand.sendHue);
    uint16_t data_hue = incomingCommand.sendHue;
    hue[REMOTELED] = (uint8_t)data_hue;
    ledChanged[REMOTELED] = true;
    //added to enable reset of fading mid fade
    isFadingRGB[REMOTELED] = false;
    fadeRGB(REMOTELED);
    startLongFade(REMOTELED);
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
#ifdef DEBUG
  Serial.print("Packet to: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
#endif
}
