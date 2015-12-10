# node-dht-sensor

This node.js module supports querying air temperature and relative humidity from a compatible DHT sensor.

## Installation
``` bash
$ npm install node-dht-sensor
```

## Usage

This module uses the [BCM2835](http://www.airspayce.com/mikem/bcm2835/) library that requires access to 
/open/mem. Because of this, you will typically run node with admin privileges.

The first step is initializing the sensor by specifying the sensor type and which GPIO pin the sensor is connected. It should work for DHT11, DHT22 and AM2302 sensors. If the initialization succeeds when you can call the read function to obtain the latest readout from the sensor. Readout values contains a temperature and a humidity property.

### First Example

This sample queries the AM2302 sensor connected to the GPIO 4 every 2 seconds and displays the result on the console. 

``` javascript
var sensorLib = require('node-dht-sensor');

var sensor = {
    initialize: function () {
        return sensorLib.initialize(22, 4);
    },
    read: function () {
        var readout = sensorLib.read();
        console.log('Temperature: ' + readout.temperature.toFixed(2) + 'C, ' +
            'humidity: ' + readout.humidity.toFixed(2) + '%');
        setTimeout(function () {
            sensor.read();
        }, 2000);
    }
};

if (sensor.initialize()) {
    sensor.read();
} else {
    console.warn('Failed to initialize sensor');
}
```

### Multiple Sensors Example

The following example shows a method for querying multiple sensors connected to the same Raspberry Pi. For this example, we have two sensors:

1. A DHT11 sensor connected to GPIO 17
2. High-resolution DHT22 sensor connected to GPIO 4

``` javascript
var sensorLib = require("node-dht-sensor");

var sensor = {
    sensors: [ {
        name: "Indoor",
        type: 11,
        pin: 17
    }, {
        name: "Outdoor",
        type: 22,
        pin: 4
    } ],
    read: function() {
        for (var a in this.sensors) {
            var b = sensorLib.readSpec(this.sensors[a].type, this.sensors[a].pin);
            console.log(this.sensors[a].name + ": " + 
              b.temperature.toFixed(1) + "C, " + 
              b.humidity.toFixed(1) + "%");
        }
        setTimeout(function() {
            sensor.read();
        }, 2000);
    }
};

sensor.read();
```


### Reference for building from source

Standard node-gyp commands are used to build the module.

1. In case, you don't have node-gyp, install it first:
   ``` bash
   $ sudo npm install node-gyp -g
   ```

2. Generate the configuration files
   ``` bash
   $ node-gyp configure
   ```

3. Build the component
   ``` bash
   $ node-gyp build
   ```

### Verbose output

Verbose output from the module can be enabled by defining ```VERBOSE``` during the module compilation. For example, this can be enabled via the binging,gyp file:

``` javascript
{
  'targets': [
    {
      'target_name': 'node-dht-sensor',
      'sources': [ 'node-dht-sensor.cpp' ],
      'libraries': [ '-lbcm2835' ],
      'defines': [ 'VERBOSE']
    }
  ]
}
```

### Appendix A: Quick Node.js installation guide

There are many ways you can get Node.js installed on your Raspberry Pi but the following method is very convenient for getting started on the latest version, very quickly.
``` shell
$ wget http://node-arm.herokuapp.com/node_latest_armhf.deb 
$ sudo dpkg -i node_latest_armhf.deb
```


### References

[1]: Node.js latest release - http://nodejs.org/dist/latest/

[2]: BCM2835 - http://www.airspayce.com/mikem/bcm2835/

[3]: Node.js native addon build tool - https://github.com/TooTallNate/node-gyp

### Build Procedure

I success at Raspberry PI 2. But I got some problems with installing node-dht-sensor module.

Step:

1.I install stable nodejs version, which includes node: 4.2.3, npm: 2.14.7. (P.S. Despite I install nodejs on my Raspberry PI Home folder, I do not use the current nodejs binary. That is, I use the older version nodejs with original on Raspberry PI (v0.10.29) and npm version 2.14.7. So it failed.

2.Using npm to install node-gyp. (with -g or not. TBD)

3.Download Broadcom Library on your board.
URL is http://www.airspayce.com/mikem/bcm2835/bcm2835-1.46.tar.gz
 - tar it
 - ./configure
 - make CFLAGS='-g -O2 -fPIC'
 - sudo make check
 - sudo make install

4.I can install node-dht-sensor with npm successfully. 
