#define NODE_ADDON_API_DISABLE_DEPRECATED

#include <napi.h>
#include <unistd.h>
#include <mutex>
#include "dht-sensor.h"
#include "util.h"

extern int initialized;
extern unsigned long long last_read[32];
extern float last_temperature[32];
extern float last_humidity[32];

std::mutex sensorMutex;

int _gpio_pin = 4;
int _sensor_type = 11;
int _max_retries = 3;
bool _test_fake_enabled = false;
float _fake_temperature = 0;
float _fake_humidity = 0;

long readSensor(int type, int pin, float &temperature, float &humidity)
{
  if (_test_fake_enabled)
  {
    temperature = _fake_temperature;
    humidity = _fake_humidity;
    return 0;
  }
  else
  {
    return readDHT(type, pin, temperature, humidity);
  }
}

class ReadWorker : public Napi::AsyncWorker
{
public:
  ReadWorker(Napi::Function &callback, int sensor_type, int gpio_pin)
      : Napi::AsyncWorker(callback), sensor_type(sensor_type),
        gpio_pin(gpio_pin) {}

  void Execute() override
  {
    sensorMutex.lock();

    if (Init()) {
      Read();
    } else {
      SetError("failed to initialize sensor");
    }

    sensorMutex.unlock();
  }

  void OnOK() override
  {
    Napi::Env env = Env();

    if (!initialized)
    {
      SetError("failed to initialize sensor");
    }
    else if (failed)
    {
      SetError("failed to read sensor");
    }
    else
    {
      Callback().Call({env.Null(),
                       Napi::Number::New(env, temperature),
                       Napi::Number::New(env, humidity)});
    }
  }

private:
  int sensor_type;
  int gpio_pin;
  bool failed = false;
  float temperature = 0;
  float humidity = 0;

  bool Init()
  {
    if (!initialized)
    {
      initialized = initialize() == 0;
    }

    return initialized;
  }

  void Read()
  {
    if (sensor_type != 11 && sensor_type != 22)
    {
      SetError("sensor type is invalid");
      return;
    }

    temperature = last_temperature[gpio_pin],
    humidity = last_humidity[gpio_pin];
    int retry = _max_retries;
    int result = 0;
    while (true)
    {
      result = readSensor(sensor_type, gpio_pin, temperature, humidity);
      if (result == 0 || --retry < 0)
        break;
      usleep(450000);
    }
    failed = result != 0;
  }
};

Napi::Value ReadAsync(const Napi::CallbackInfo &info)
{
  int sensor_type = info[0].ToNumber().Uint32Value();
  int gpio_pin = info[1].ToNumber().Uint32Value();
  auto callback = info[2].As<Napi::Function>();

  auto worker = new ReadWorker(callback, sensor_type, gpio_pin);
  worker->Queue();

  return info.Env().Undefined();
}

Napi::Value ReadSync(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  int sensor_type;
  int gpio_pin;

  if (info.Length() == 2)
  {
    gpio_pin = info[1].ToNumber().Uint32Value();
    // TODO: validate gpio_pin

    sensor_type = info[0].ToNumber().Uint32Value();
    if (sensor_type != 11 && sensor_type != 22)
    {
      Napi::TypeError::New(env, "specified sensor type is invalid").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    // initialization (on demand)
    if (!initialized)
    {
      initialized = initialize() == 0;
      if (!initialized)
      {
        Napi::TypeError::New(env, "failed to initialize").ThrowAsJavaScriptException();
        return env.Undefined();
      }
    }
  }
  else
  {
    sensor_type = _sensor_type;
    gpio_pin = _gpio_pin;
  }

  float temperature = last_temperature[gpio_pin],
        humidity = last_humidity[gpio_pin];
  int retry = _max_retries;
  int result = 0;
  while (true)
  {
    result = readSensor(sensor_type, gpio_pin, temperature, humidity);
    if (result == 0 || --retry < 0)
      break;
    usleep(450000);
  }

  auto readout = Napi::Object::New(env);
  readout.Set("humidity", humidity);
  readout.Set("temperature", temperature);
  readout.Set("isValid", result == 0);
  readout.Set("errors", _max_retries - retry);

  return readout;
}

Napi::Value Read(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  switch (info.Length())
  {
  case 0: // no parameters, use synchronous interface
  case 2: // sensor type and GPIO pin, use synchronous interface
    return ReadSync(info);
  case 3:
    // sensorType, gpioPin and callback, use asynchronous interface
    return ReadAsync(info);
  default:
    Napi::TypeError::New(env, "invalid number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

void SetMaxRetries(const Napi::CallbackInfo &info)
{
  if (info.Length() != 1)
  {
    Napi::TypeError::New(info.Env(), "Wrong number of arguments").ThrowAsJavaScriptException();
    return;
  }
  _max_retries = info[0].ToNumber().Uint32Value();
}

void LegacyInitialization(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!info[0].IsNumber() || !info[1].IsNumber())
  {
    Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
    return;
  }

  int sensor_type = info[0].ToNumber().Uint32Value();
  if (sensor_type != 11 && sensor_type != 22)
  {
    Napi::TypeError::New(env, "Specified sensor type is not supported").ThrowAsJavaScriptException();
    return;
  }

  // update parameters
  _sensor_type = sensor_type;
  _gpio_pin = info[1].ToNumber().Uint32Value();

  if (info.Length() >= 3)
  {
    if (!info[2].IsNumber())
    {
      Napi::TypeError::New(env, "Invalid maxRetries parameter").ThrowAsJavaScriptException();
      return;
    }
    else
    {
      _max_retries = info[2].ToNumber().Uint32Value();
    }
  }
}

Napi::Value Initialize(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (info.Length() > 1)
  {
    LegacyInitialization(info);
    return Napi::Boolean::New(env, initialized || initialize() == 0);
  }
  else if (!info[0].IsObject())
  {
    Napi::TypeError::New(env, "Invalid argument: an object is expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Object options = info[0].ToObject();

  const auto KEY_TEST = "test";
  const auto KEY_FAKE = "fake";
  const auto KEY_TEMP = "temperature";
  const auto KEY_HUMIDITY = "humidity";

  Napi::Value testKeyValue = objectGetDefined(options, KEY_TEST);
  if (!testKeyValue.IsEmpty())
  {
    Napi::Object testKey = testKeyValue.ToObject();
    initialized = 1;

    Napi::Value fakeKeyValue = objectGetDefined(testKey, KEY_FAKE);
    if (fakeKeyValue.IsEmpty())
    {
      Napi::TypeError::New(env, "Invalid argument: 'options.test.fake' is missing or is not an object").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    Napi::Object fakeKey = fakeKeyValue.ToObject();
    _test_fake_enabled = true;

    if (fakeKey.Has(KEY_TEMP))
    {
      Napi::Value temp = fakeKey.Get(KEY_TEMP);
      if (!temp.IsNumber())
      {
        Napi::TypeError::New(env, "Invalid argument: 'options.test.fake.temperature' must be a number")
            .ThrowAsJavaScriptException();
        return env.Undefined();
      }

      _fake_temperature = temp.As<Napi::Number>().FloatValue();
    }
    else
    {
      Napi::Error::New(env, "Test mode: temperature value must be defined for a fake").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    if (fakeKey.Has(KEY_HUMIDITY))
    {
      Napi::Value humidity = fakeKey.Get(KEY_HUMIDITY);
      if (!humidity.IsNumber())
      {
        Napi::TypeError::New(env, "Invalid argument: 'options.test.fake.humidity' must be a number")
            .ThrowAsJavaScriptException();
        return env.Undefined();
      }

      _fake_humidity = humidity.As<Napi::Number>().FloatValue();
    }
    else
    {
      Napi::Error::New(env, "Test mode: humidity value must be defined for a fake").ThrowAsJavaScriptException();
      return env.Undefined();
    }
  }

  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set("read", Napi::Function::New(env, Read));
  exports.Set("initialize", Napi::Function::New(env, Initialize));
  exports.Set("setMaxRetries", Napi::Function::New(env, SetMaxRetries));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
