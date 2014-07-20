#include <node.h>

#include <cstdlib>
#include <ctime>

// Access from ARM running linux
#define BCM2708_PERI_BASE	0x20000000
#define GPIO_BASE		(BCM2708_PERI_BASE + 0x200000)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <bcm2835.h>
#include <unistd.h>
#include <sched.h>

#define MAXTIMINGS 100

#define DHT11  11
#define DHT22  22
#define AM2302 22

#ifdef VERBOSE
int bits[1000];
int bitidx = 0;
#endif
int data[100];

long readDHT(int type, int pin, float &temperature, float &humidity)
{
    int counter = 0;
    int laststate = HIGH;
    int j=0;
//    bitidx = 0;

    // Set GPIO pin to output
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    
    bcm2835_gpio_write(pin, HIGH);
    bcm2835_delay(400);
    bcm2835_gpio_write(pin, LOW);
    bcm2835_delay(20);
    
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;
    
    // wait for pin to drop?
    int timeout = 100000;
    while (bcm2835_gpio_lev(pin) == 1) {
        if (--timeout < 0) {
            return -3;
        }
        bcm2835_delayMicroseconds(1); //usleep(1);
    }
    
    // read data!
    for (int i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (bcm2835_gpio_lev(pin) == laststate) {
            counter++;
            if (counter == 1000)
                break;
        }
        laststate = bcm2835_gpio_lev(pin);
        if (counter == 1000) break;
/*#ifdef VERBOSE
        if (bitidx < 1000) {
            bits[bitidx++] = counter;
        } else {
            printf("WARNING: bits buffer blew up!");
        }
#endif*/
        
        if ((i>3) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            data[j/8] <<= 1;
            if (counter > 200)
                data[j/8] |= 1;
            j++;
        }
    }
    
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
    
    return 0;
}

int initialize()
{
    // set up real-time scheduling
    struct sched_param schedp;
    schedp.sched_priority = 1;
    sched_setscheduler(0, SCHED_FIFO, &schedp);

    if (!bcm2835_init())
        return 1;
    else
        return 0;
}

using namespace v8;

int GPIOPort = 4;
int SensorType = 11;

Handle<Value> Read(const Arguments& args) {
    HandleScope scope;
    
    float temperature, humidity;
    int retry = 3;
    int result = 0;
    do {
        result = readDHT(SensorType, GPIOPort, temperature, humidity);
        if (--retry < 0) break;
    } while (result != 0);

    Local<Object> readout = Object::New();
    readout->Set(String::NewSymbol("humidity"), Number::New(humidity));
    readout->Set(String::NewSymbol("temperature"), Number::New(temperature));
    readout->Set(String::NewSymbol("isValid"), Boolean::New(result == 0));
    readout->Set(String::NewSymbol("errors"), Number::New(2 - retry));
    return scope.Close(readout);
}

Handle<Value> Initialize(const Arguments& args) {
    HandleScope scope;
    
    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }
    
    if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong arguments")));
        return scope.Close(Undefined());
    }
    
    int sensorType = args[0]->Uint32Value();
    if (sensorType != 11 && sensorType != 22) {
        ThrowException(Exception::TypeError(String::New("Specified sensor type is not supported")));
        return scope.Close(Undefined());
    }
    
    // update parameters
    SensorType = sensorType;
    GPIOPort = args[1]->Uint32Value();
    
    return scope.Close(v8::Boolean::New(initialize() == 0));
}

void RegisterModule(v8::Handle<v8::Object> target) {
    target->Set(String::NewSymbol("read"),
                FunctionTemplate::New(Read)->GetFunction());
    target->Set(String::NewSymbol("initialize"),
                FunctionTemplate::New(Initialize)->GetFunction());
}

NODE_MODULE(node_dht_sensor, RegisterModule);
