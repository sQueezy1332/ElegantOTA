#pragma once
#include <Arduino.h>
#include <FS.h>
#include <StreamString.h>
#include "ESPAsyncWebServer.h"
#if defined(ESP8266)
#include "LittleFS.h"
#include "Updater.h"
extern UpdaterClass Update;
#elif defined(ESP32)
#include <Update.h>
extern UpdateClass Update;
#endif
#ifndef OTA_FILE_PATH
#define OTA_FILE_PATH "/ota.html"
#endif //without .gz for automatic add gzip header in AsyncFileResponse
extern fs::SPIFFSFS SPIFFS;
__weak_symbol extern bool auth_handler(AsyncWebServerRequest*&);
/*bool auth_handler(AsyncWebServerRequest*& request) {
	if (*_login.c_str()) {
		if (!request->authenticate(_login.c_str(), _password.c_str())) {
			request->requestAuthentication();
			return false;
		}
	}
	return true;
}*/

namespace ota {
	typedef void(*progress_cb)(size_t prog, size_t size);
	enum OTA_Mode {
		OTA_MODE_FIRMWARE = U_FLASH,
		OTA_MODE_FILESYSTEM = 100
	};

	void printError(AsyncWebServerRequest*& request) {//1235876
		StreamString error_str;
		Update.printError(error_str);
		log_e("%s", error_str.c_str());
		request->send(400, "text/plain", error_str.c_str());
	}

	void onStart(AsyncWebServerRequest* request) {
		if (&auth_handler && !auth_handler(request)) return;
		OTA_Mode mode = OTA_MODE_FIRMWARE; // Get header x-ota-mode value, if present
		// Get mode from arg
		if (request->hasParam("mode")) {
			const String& str = request->getParam("mode")->value();
			if (str == "fs") {
				mode = OTA_MODE_FILESYSTEM;
			}
			else { mode = OTA_MODE_FIRMWARE; }
			log_v("OTA Mode: %u", str.c_str());
		}
		// Get file MD5 hash from arg
		if (request->hasParam("hash")) {
			const String& hash = request->getParam("hash")->value();
			log_v("MD5: %s", hash.c_str());
			if (!Update.setMD5(hash.c_str())) {
				printError(request);
				return;
			}
		}
		//if (preUpdateCallback) preUpdateCallback(); // Start OTA update callback
		uint32_t update_size;
#if defined(ESP8266)
		if (mode == OTA_MODE_FILESYSTEM) {
			update_size = ((size_t)FS_end - (size_t)FS_start);
			close_all_fs();
		}
		else { update_size = ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000); }
		Update.runAsync(true);
#elif defined(ESP32) 
		update_size = UPDATE_SIZE_UNKNOWN;
		if (!Update.begin(update_size, mode)) {
			printError(request);
			return;
		}
#endif
		request->send(200, "text/plain", "OK");
	}

	void onRequest(AsyncWebServerRequest* request) {
		//if (_authenticate && !auth_handler(request)) return;
		//if (postUpdateCallback) postUpdateCallback(!Update.hasError());// End OTA update callback
		auto ret = Update.hasError();
		auto response = request->beginResponse(ret ? 400 : 200, "text/plain", ret ? asyncsrv::empty : "OK");
		response->addHeader("Connection", "close");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
	}

	void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool complete) {
		if (&auth_handler && !auth_handler(request)) return;
		//if (!index) _current_progress_size = 0; // Reset progress size on first frame
		if (len) { // Write chunked data to the free sketch space
			if (Update.write(data, len) != len) {
				log_e("Failed to write chunked data to free space");
				return request->send(400, "text/plain", "Failed to write chunked data to free space");
			}
			//_current_progress_size += len;
			//if (progressUpdateCallback) progressUpdateCallback(_current_progress_size, request->contentLength());// Progress update callback
		}
		if (complete) { // if the final flag is set - this is the last frame of data
			if (!Update.end(true)) printError(request); //true to set the size to the current progress
		}
	}

	void server_init(AsyncWebServer& server, progress_cb fun = nullptr) {
		server.on("/update", HTTP_GET, [&](AsyncWebServerRequest* request) {
			if (&auth_handler && !auth_handler(request)) return;
			//AsyncWebServerResponse* response = request->beginResponse(SPIFFS, OTA_FILE_PATH, "text/html");
			//response->addHeader("Content-Encoding", "gzip");
			//request->send(response); asyncsrv::T__gz; asyncsrv::T_Content_Encoding;
			request->send(SPIFFS, OTA_FILE_PATH, "text/html");
			});
		server.on("/ota/start", HTTP_GET, onStart);
		server.on("/ota/upload", HTTP_POST, onRequest, onUpload);
		Update.onProgress(fun);//sizeof(UpdateClass::THandlerFunction_Progress);
	}
}
