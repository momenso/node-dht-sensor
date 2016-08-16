#include <node.h>

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <bcm2835.h>
#include <sched.h>
#include <unistd.h>

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)

#define MAXTIMINGS          100 // 100

#define DHT11               11
#define DHT22               22
#define AM2302              22

#ifdef VERBOSE
int bits[1000+1];
int bitidx = 0;
#endif
int initialized = 0;
int data[100];
unsigned long long last_read[32];
float last_temperature[32] = {};
float last_humidity[32] = {};

void set_default_priority(void) {
  struct sched_param sched;
  memset(&sched, 0, sizeof(sched));
  // Go back to default scheduler with default 0 priority.
  sched.sched_priority = 0;
  sched_setscheduler(0, SCHED_OTHER, &sched);
}

void set_max_priority(void) {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance 
	// of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

unsigned long long getTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long time = (unsigned long long)(tv.tv_sec)*1000 +
                            (unsigned long long)(tv.tv_usec)/1000;
  return time;
}

long readDHT(int type, int pin, float &temperature, float &humidity)
{
    int laststate = HIGH;
    int j = 0;
	int timeout;
    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

#ifdef VERBOSE
    bitidx = 0;
#endif

    unsigned long long now = getTime();
    if (now - last_read[pin] < 2000) {
#ifdef VERBOSE
       printf("Too early to read again pin %d: %llu\n", pin, now - last_read[pin]);
#endif
       temperature = last_temperature[pin];
       humidity = last_humidity[pin];
       return 0;
    } else {
       last_read[pin] = now + 420;
    }

    // set up real-time scheduling
    set_max_priority();

    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(pin, HIGH);
	usleep(10000);
    bcm2835_gpio_write(pin, LOW);
	usleep(type == 11 ? 2500 : 1000); // 1000, 900
	bcm2835_gpio_write(pin, HIGH);

	//bcm2835_gpio_write(pin, HIGH);

	//usleep(20);
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

	for (timeout = 0; timeout < 100000 && bcm2835_gpio_lev(pin) == 0; timeout++);
	if (timeout >= 100000) return -3;
	//while (bcm2835_gpio_lev(pin) == 0);
	for (timeout = 0; timeout < 100000 && bcm2835_gpio_lev(pin) == 1; timeout++);
	if (timeout >= 100000) return -3;
	//while (bcm2835_gpio_lev(pin) == 1);

    // read data!
    for (int i = 0; i < MAXTIMINGS; ++i) {
        int counter = 0;
        while (bcm2835_gpio_lev(pin) == laststate) {
            if (++counter >= 800) break;
        }
        laststate = bcm2835_gpio_lev(pin);
        if (counter >= 800) break;
/*#ifdef VERBOSE
        if (bitidx < 1000) {
            bits[bitidx++] = counter;
        } else {
            printf("WARNING: bits buffer blew up!\n");
        }
#endif*/
        
        if ((i > 3) && (i % 2 == 0)) {
            // shove each bit into the storage bytes
            data[j / 8] <<= 1;
            if (counter >= 230) { // > 200
                data[j / 8] |= 1;
			}
            ++j;
        }
    }

	set_default_priority();
    
    if ((j >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xff)))
    {
#ifdef VERBOSE
        printf("[Sensor type = %d] ", type);
#endif

        if (type == DHT11) {
#ifdef VERBOSE
            printf("Temp = %d C, Hum = %d %%\n", data[2], data[0]);
#endif
            temperature = data[2];
            humidity = data[0];
        }
        else if (type == DHT22)
        {
            float f, h;
            h = data[0] * 256 + data[1];
            h /= 10;
            
            f = (data[2] & 0x7F) * 256 + data[3];
            f /= 10.0;
            if (data[2] & 0x80) f *= -1;
            
#ifdef VERBOSE
            printf("Temp = %.1f C, Hum = %.1f %%\n", f, h);
#endif
            temperature = f;
            humidity = h;
        }
        else
        {
            return -2;
        }
    }
    else
    {
#ifdef VERBOSE
        printf("Unexpected data: j=%d: %d != %d + %d + %d + %d\n",
               j, data[4], data[0], data[1], data[2], data[3]);
#endif
        return -1;
    }
    
#ifdef VERBOSE
    printf("Obtained readout successfully.\n");
#endif

    // update last readout
    last_temperature[pin] = temperature;
    last_humidity[pin] = humidity;

    return 0;
}

int initialize()
{
    if (!bcm2835_init())
    {
#ifdef VERBOSE
        printf("BCM2835 initialization failed.\n");
#endif
        return 1;
    }
    else
    {
#ifdef VERBOSE
        printf("BCM2835 initialized.\n");
#endif
        initialized = 1;
        memset(last_read, 0, sizeof(unsigned long long)*32);
        return 0;
    }
}

#include <nan.h>

using namespace v8;

int GPIOPort = 4;
int SensorType = 11;

void Read(const Nan::FunctionCallbackInfo<Value>& args) {
    float temperature = last_temperature[GPIOPort], 
          humidity = last_humidity[GPIOPort];
    int retry = 3;
    int result = 0;
    do {
        result = readDHT(SensorType, GPIOPort, temperature, humidity);
        if (--retry < 0) break;
		last_read[GPIOPort] = 0;
		usleep(2000000);
    } while (result != 0);

    Local<Object> readout = Nan::New<Object>();
    readout->Set(Nan::New("humidity").ToLocalChecked(), Nan::New<Number>(humidity));
    readout->Set(Nan::New("temperature").ToLocalChecked(), Nan::New<Number>(temperature));
    readout->Set(Nan::New("isValid").ToLocalChecked(), Nan::New<Boolean>(result == 0));
    readout->Set(Nan::New("errors").ToLocalChecked(), Nan::New<Number>(2 - retry));

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
