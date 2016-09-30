#include <node.h>
#include <nan.h>
#include <unistd.h>
#include <mutex>
#include "dht-sensor.h"

using namespace v8;

extern int initialized;
extern unsigned long long last_read[32];
extern float last_temperature[32];
extern float last_humidity[32];

std::mutex sensorMutex;

int GPIOPort = 4;
int SensorType = 11;

class ReadWorker : public Nan::AsyncWorker {
  public:
    ReadWorker(Nan::Callback *callback, int sensor_type, int gpio_pin)
      : Nan::AsyncWorker(callback), sensor_type(sensor_type),
        gpio_pin(gpio_pin) { }

    void Execute() {
      sensorMutex.lock();
      Init();
      Read();
      sensorMutex.unlock();
    }

    void HandleOKCallback() {
      Nan:: HandleScope scope;

      Local<Value> argv[3];
      argv[1] = Nan::New<Number>(temperature),
      argv[2] = Nan::New<Number>(humidity);

      if (!initialized) {
        argv[0] = Nan::Error("failed to initialize sensor");
      } else if (failed) {
        argv[0] = Nan::Error("failed to read sensor");
      } else {
        argv[0] = Nan::Null();
      }

      callback->Call(3, argv);
    }

  private:
    int sensor_type;
    int gpio_pin;
    bool failed = false;
    float temperature = 0;
    float humidity = 0;

    void Init() {
      if (!initialized) {
        initialized = initialize() == 0;
      }
    }

    void Read() {
      if (sensor_type != 11 && sensor_type != 22) {
        SetErrorMessage("sensor type is invalid");
        return;
      }

      int retry = 3;
      int result = 0;
      while (true) {
        result = readDHT(sensor_type, gpio_pin, temperature, humidity);
  			if (result == 0 || --retry <= 0) break;
  			last_read[GPIOPort] = 0;
  			sleep(1);
      }
      failed = result != 0;
    }
};

void Read(const Nan::FunctionCallbackInfo<Value>& args) {
  int params = args.Length();
  if (params == 3) {
    int sensor_type = Nan::To<int>(args[0]).FromJust();
    int gpio_pin = Nan::To<int>(args[1]).FromJust();
    Nan::Callback *callback = new Nan::Callback(args[2].As<Function>());

    Nan::AsyncQueueWorker(new ReadWorker(callback, sensor_type, gpio_pin));
  }
  else if (params == 2)
  {
    int sensorType = args[0]->Uint32Value();
    if (sensorType != 11 && sensorType != 22) {
      Nan::ThrowTypeError("specified sensor type is invalid");
      return;
    }

    if (!initialized) {
      initialized = initialize() == 0;
      if (!initialized) {
        Nan::ThrowTypeError("failed to initialize");
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
  else if (params == 0)
  {
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

    args.GetReturnValue().Set(readout);
  }
  else
  {
    Nan::ThrowTypeError("invalid number of arguments");
  }
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
	Nan::SetMethod(exports, "initialize", Initialize);
}

NODE_MODULE(node_dht_sensor, Init);
