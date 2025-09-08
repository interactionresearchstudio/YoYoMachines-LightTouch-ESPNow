// ---------- HTML ----------
String pageHtml(const String& current) {
  String h;
  h.reserve(2048);
  h += F(
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32 Setup</title>"
    "<style>body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial;"
    "margin:2rem;line-height:1.4}input[type=text]{width:100%;padding:.7rem;"
    "font-size:1rem;border:1px solid #ccc;border-radius:.5rem}"
    "button{margin-top:1rem;padding:.7rem 1rem;font-size:1rem;border:0;"
    "border-radius:.5rem;background:#4a90e2;color:white;cursor:pointer;display:block}"
    ".card{max-width:540px;margin:auto;border:1px solid #eee;padding:1rem;"
    "border-radius:.75rem;box-shadow:0 2px 10px rgba(0,0,0,.06)}"
    ".small{color:#666;font-size:.9rem;margin-top:.5rem}</style></head><body>"
    "<div class='card'><h2>YoYo Machines Setup</h2>"
    "<form method='POST' action='/save'>"
    "<label for='text'>Enter text to store:</label><br>"
    "<input id='text' name='text' type='text' placeholder='Type here…' autofocus "
    "value='");
  // escape minimal HTML chars
  for (char c : current) {
    if (c == '\'') h += "&#39;"; 
    else if (c == '<') h += "&lt;"; 
    else if (c == '&') h += "&amp;"; 
    else h += c;
  }
  h += F(
    "'><button type='submit'>Save</button></form>"
    "<form method='POST' action='/test'>"
    "<button type='submit' style='background:#27ae60'>Test LED Blink</button>"
    "</form>"
    "<div class='small'>Unique Channel Code. Make sure both devices codes match!"
    "Close this page after saving.</div></div></body></html>");
  return h;
}

// ---------- Handlers ----------
void handleRoot() {
  server.send(200, "text/html", pageHtml(savedText));
}

void handleSave() {
  if (!server.hasArg("text")) {
    server.send(400, "text/plain", "Missing 'text' parameter");
    return;
  }
  savedText = server.arg("text");
  prefs.putString("text", savedText);
  server.send(200, "text/html",
              F("<!doctype html><html><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<body style='font-family:system-ui;margin:2rem'>"
                "<h2>Saved!</h2>"
                "<p>Your text has been stored persistently.</p>"
                "<p><a href='/'>Back</a></p>"
                "</body></html>"));
  delay(500);
  factoryReset();
}

void handleTest() {
  server.send(200, "text/html",
              F("<!doctype html><html><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<body style='font-family:system-ui;margin:2rem'>"
                "<h2>Testing LED</h2>"
                "<p>The onboard LED should blink now.</p>"
                "<p><a href='/'>Back</a></p>"
                "</body></html>"));

  // Blink LED asynchronously
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
}

void handleNotFound() {
  // Captive portal redirect
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleProbe() {
  server.send(200, "text/html", pageHtml(savedText));
}

// ---------- Portal control ----------
void startPortal() {
  portalIsOpen = true;

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("YOYO-"+String(myID), "12345678")) {
    Serial.println("AP start failed");
    return;
  }
  delay(100);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP up at: "); Serial.println(apIP);

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", apIP);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/test", HTTP_POST, handleTest);

  server.on("/generate_204", HTTP_ANY, handleProbe); // Android
  server.on("/gen_204", HTTP_ANY, handleProbe);
  server.on("/hotspot-detect.html", HTTP_ANY, handleProbe); // iOS/macOS
  server.on("/ncsi.txt", HTTP_ANY, handleProbe);            // Windows
  server.onNotFound(handleNotFound);

  server.begin();

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Portal started.");
}

void stopPortal() {
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Portal stopped.");
  factoryReset();

}

void portalHandler(){
  dns.processNextRequest();
  server.handleClient();
}