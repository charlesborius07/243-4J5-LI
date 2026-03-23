#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <Client.h>
#include <ESP_SSLClient.h>
#include <mbedtls/base64.h>

class WebSocketClient : public Client {
private:
  ESP_SSLClient* _sslClient;
  bool _wsConnected;
  String _mqttPath;

  uint8_t _rxBuffer[512];
  size_t _rxBufferLen;
  size_t _rxBufferPos;

  String generateWebSocketKey() {
    uint8_t key[16];
    for(int i = 0; i < 16; i++) {
      key[i] = random(0, 256);
    }
    size_t olen;
    unsigned char output[64];
    mbedtls_base64_encode(output, sizeof(output), &olen, key, 16);
    return String((char*)output);
  }

  bool readWebSocketFrame() {
    if (!_sslClient->available()) return false;

    uint8_t byte1 = _sslClient->read();
    if (!_sslClient->available()) return false;
    uint8_t byte2 = _sslClient->read();

    uint8_t opcode = byte1 & 0x0F;
    bool masked = (byte2 & 0x80) != 0;
    size_t payloadLen = byte2 & 0x7F;

    if (payloadLen == 126) {
      if (_sslClient->available() < 2) return false;
      payloadLen = (_sslClient->read() << 8) | _sslClient->read();
    } else if (payloadLen == 127) {
      if (_sslClient->available() < 8) return false;
      payloadLen = 0;
      for(int i = 0; i < 8; i++) {
        payloadLen = (payloadLen << 8) | _sslClient->read();
      }
    }

    uint8_t mask[4] = {0};
    if (masked) {
      if (_sslClient->available() < 4) return false;
      for(int i = 0; i < 4; i++) {
        mask[i] = _sslClient->read();
      }
    }

    if (opcode == 0x01 || opcode == 0x02) { // Text ou Binary
      if (_sslClient->available() < payloadLen) return false;

      _rxBufferLen = payloadLen < sizeof(_rxBuffer) ? payloadLen : sizeof(_rxBuffer);
      for(size_t i = 0; i < _rxBufferLen; i++) {
        _rxBuffer[i] = _sslClient->read();
        if (masked) _rxBuffer[i] ^= mask[i % 4];
      }
      _rxBufferPos = 0;
      return true;
    }
    else if (opcode == 0x08) { // Close
      Serial.println("[WSS] Serveur a ferme la connexion");
      _wsConnected = false;
      return false;
    }
    else if (opcode == 0x09) { // Ping
      uint8_t pong[2] = {0x8A, 0x00};
      _sslClient->write(pong, 2);
      return false;
    }

    return false;
  }

public:
  WebSocketClient(ESP_SSLClient* sslClient, const char* path = "/") {
    _sslClient = sslClient;
    _mqttPath = path;
    _wsConnected = false;
    _rxBufferLen = 0;
    _rxBufferPos = 0;
  }

  int connect(IPAddress ip, uint16_t port) override { return 0; }
  int connect(const char *host, uint16_t port) override {
    Serial.println("[WSS] Connexion SSL...");

    if (!_sslClient->connect(host, port)) {
      Serial.println("[WSS] Echec connexion SSL");
      return 0;
    }

    Serial.println("[WSS] SSL connecte, envoi handshake WebSocket...");
    String wsKey = generateWebSocketKey();

    _sslClient->print("GET ");
    _sslClient->print(_mqttPath);
    _sslClient->print(" HTTP/1.1\r\nHost: ");
    _sslClient->print(host);
    _sslClient->print("\r\nUpgrade: websocket\r\n");
    _sslClient->print("Connection: Upgrade\r\n");
    _sslClient->print("Sec-WebSocket-Key: ");
    _sslClient->print(wsKey);
    _sslClient->print("\r\nSec-WebSocket-Protocol: mqtt\r\n");
    _sslClient->print("Sec-WebSocket-Version: 13\r\n\r\n");

    unsigned long timeout = millis();
    while (!_sslClient->available() && millis() - timeout < 5000) {
      delay(10);
    }

    if (!_sslClient->available()) {
      Serial.println("[WSS] Timeout handshake");
      return 0;
    }

    String response = "";
    while (_sslClient->available()) {
      char c = _sslClient->read();
      response += c;
      if (response.endsWith("\r\n\r\n")) break;
    }

    if (response.indexOf("101") > 0 && response.indexOf("Switching Protocols") > 0) {
      Serial.println("[WSS] Handshake WebSocket reussi!");
      _wsConnected = true;
      return 1;
    } else {
      Serial.println("[WSS] Handshake WebSocket echoue");
      return 0;
    }
  }

  size_t write(uint8_t b) override {
    return write(&b, 1);
  }

  size_t write(const uint8_t *buf, size_t size) override {
    if (!_wsConnected) return 0;

    uint8_t header[14];
    int headerLen = 2;

    header[0] = 0x82; // FIN + Binary frame

    if (size < 126) {
      header[1] = 0x80 | size;
    } else if (size < 65536) {
      header[1] = 0x80 | 126;
      header[2] = (size >> 8) & 0xFF;
      header[3] = size & 0xFF;
      headerLen = 4;
    } else {
      header[1] = 0x80 | 127;
      for(int i = 0; i < 8; i++) header[2 + i] = 0;
      header[6] = (size >> 24) & 0xFF;
      header[7] = (size >> 16) & 0xFF;
      header[8] = (size >> 8) & 0xFF;
      header[9] = size & 0xFF;
      headerLen = 10;
    }

    uint8_t mask[4];
    for(int i = 0; i < 4; i++) {
      mask[i] = random(0, 256);
      header[headerLen + i] = mask[i];
    }
    headerLen += 4;

    _sslClient->write(header, headerLen);

    for(size_t i = 0; i < size; i++) {
      uint8_t maskedByte = buf[i] ^ mask[i % 4];
      _sslClient->write(&maskedByte, 1);
    }

    return size;
  }

  int available() override {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBufferLen - _rxBufferPos;
    }

    if (_sslClient->available()) {
      if (readWebSocketFrame()) {
        return _rxBufferLen - _rxBufferPos;
      }
    }

    return 0;
  }

  int read() override {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBuffer[_rxBufferPos++];
    }

    if (_sslClient->available()) {
      if (readWebSocketFrame() && _rxBufferPos < _rxBufferLen) {
        return _rxBuffer[_rxBufferPos++];
      }
    }

    return -1;
  }

  int read(uint8_t *buf, size_t size) override {
    size_t count = 0;
    while (count < size) {
      int c = read();
      if (c < 0) break;
      buf[count++] = (uint8_t)c;
    }
    return count;
  }

  int peek() override {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBuffer[_rxBufferPos];
    }
    return -1;
  }

  void flush() override {
    _sslClient->flush();
  }

  void stop() override {
    _wsConnected = false;
    _sslClient->stop();
  }

  uint8_t connected() override {
    return _wsConnected && _sslClient->connected();
  }

  operator bool() override {
    return _wsConnected;
  }
};

#endif
