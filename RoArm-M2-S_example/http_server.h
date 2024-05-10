#include "m2_web_page.h"

// Create AsyncWebServer object on port 80
WebServer server(80);

void handleRoot(){
  server.send(200, "text/html", index_html); //Send web page
}

void handleHorizontalDrag(){
  server.send(200, "text/html", horizontal_drag_html); //Send web page
}

void handleVerticalDrag(){
  server.send(200, "text/html", vertical_drag_html); //Send web page
}

void webCtrlServer(){
  server.on("/", handleRoot);
  server.on("/horiDrag", handleHorizontalDrag);
  server.on("/vertDrag", handleVerticalDrag);

  server.on("/js", [](){
    String jsonCmdWebString = server.arg(0);
    deserializeJson(jsonCmdReceive, jsonCmdWebString);
    jsonCmdReceiveHandler();
    serializeJson(jsonInfoHttp, jsonFeedbackWeb);
    server.send(200, "text/plane", jsonFeedbackWeb);
    jsonFeedbackWeb = "";
    jsonInfoHttp.clear();
    jsonCmdReceive.clear();
  });

  // Start server
  server.begin();
  Serial.println("Server Starts.");
}

void initHttpWebServer(){
  webCtrlServer();
}