#include <node.h>
#include <nan.h>
#include <unistd.h>
#include "dht-sensor.h"
using namespace v8;

extern int initialized;
extern unsigned long long last_read[32];
extern float last_temperature[32];
extern float last_humidity[32];
extern int data[100];

int GPIOPort = 4;
int SensorType = 11;

void Read(const Nan::FunctionCallbackInfo<Value>& args) {
    float temperature = last_temperature[GPIOPort],
          humidity = last_humidity[GPIOPort];
    int retry = 0;
    int result = 0;
    while (true) {
      result = readDHT(SensorType, GPIOPort, temperature, humidity);
			if (result == 0 || ++retry > 3) break;
			last_read[GPIOPort] = 0;
			sleep(1);
    }

    Local<Object> readout = Nan::New<Object>();
    readout->Set(Nan::New("humidity").ToLocalChecked(), Nan::New<Number>(humidity));
    readout->Set(Nan::New("temperature").ToLocalChecked(), Nan::New<Number>(temperature));
    readout->Set(Nan::New("isValid").ToLocalChecked(), Nan::New<Boolean>(result == 0));
    readout->Set(Nan::New("errors").ToLocalChecked(), Nan::New<Number>(retry));
    readout->Set(Nan::New("raw").ToLocalChecked(), Nan::New<Number>(
      ((long long)data[0] << 32) |
      ((long long)data[1] << 24) |
      ((long long)data[2] << 16) |
      ((long long)data[3] << 8) |
      ((long long)data[4])
    ));

    args.GetReturnValue().Set(readout);
}

void ReadSpec(const Nan::FunctionCallbackInfo<Value>& args) {
    if (args.Length() < 2) {
    	Nan::ThrowTypeError("Wrong number of arguments");
    	return;
    }

    int sensorType = args[0]->Uint32Value();
    if (sensorType != 11 && sensorType != 22) {
      Nan::ThrowTypeError("Specified sensor type is invalid");
      return;
    }

    if (!initialized) {
      initialized = initialize() == 0;
      if (!initialized) {
      	Nan::ThrowTypeError("Failed to initialize");
      	return;
      }
    }

    int gpio = args[1]->Uint32Value();
    float temperature = 0, humidity = 0;
    int retry = 3;
    int result = 0;
    do {
      result = readDHT(sensorType, gpio, temperature, humidity);
      if (--retry < 0) break;
    } while (result != 0);

    Local<Object> readout = Nan::New<Object>();
    readout->Set(Nan::New("humidity").ToLocalChecked(), Nan::New<Number>(humidity));
    readout->Set(Nan::New("temperature").ToLocalChecked(), Nan::New<Number>(temperature));
    readout->Set(Nan::New("isValid").ToLocalChecked(), Nan::New<Boolean>(result == 0));
    readout->Set(Nan::New("errors").ToLocalChecked(), Nan::New<Number>(2 - retry));

    args.GetReturnValue().Set(readout);
}

void Initialize(const Nan::FunctionCallbackInfo<Value>& args) {
    if (args.Length() < 2) {
			Nan::ThrowTypeError("Wrong number of arguments");
			return;
    }

    if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
			Nan::ThrowTypeError("Invalid arguments");
			return;
    }

    int sensorType = args[0]->Uint32Value();
    if (sensorType != 11 && sensorType != 22) {
    	Nan::ThrowTypeError("Specified sensor type is not supported");
    	return;
    }

    // update parameters
    SensorType = sensorType;
    GPIOPort = args[1]->Uint32Value();

    args.GetReturnValue().Set(Nan::New<Boolean>(initialize() == 0));
}

void Init(Handle<Object> exports) {
	Nan::SetMethod(exports, "read", Read);
	Nan::SetMethod(exports, "readSpec", ReadSpec);
	Nan::SetMethod(exports, "initialize", Initialize);
}

NODE_MODULE(node_dht_sensor, Init);
