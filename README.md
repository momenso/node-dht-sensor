# node-dht-sensor

A simple node.js module for reading temperature and relative humidity using a compatible DHT sensor.

[![Build Status](https://travis-ci.org/momenso/node-dht-sensor.svg?branch=master)](https://travis-ci.org/momenso/node-dht-sensor)
[![npm](https://img.shields.io/npm/v/node-dht-sensor.svg?label=npm%20package)](https://www.npmjs.com/package/node-dht-sensor)
[![npm](https://img.shields.io/npm/dm/node-dht-sensor.svg)](https://www.npmjs.com/package/node-dht-sensor)
[![LICENSE](https://img.shields.io/github/license/momenso/node-dht-sensor.svg)](https://github.com/momenso/node-dht-sensor/blob/master/LICENSE)

## Installation

```shell session
$ npm install node-dht-sensor
```

Please note that differently from versions 0.0.x there's no need to pre-install the BCM2835 library [2].

## Usage

To initialize the sensor, you have to specify the sensor type and the [GPIO pin](https://www.raspberrypi.org/documentation/usage/gpio/) where the sensor is connected to. It should work for DHT11, DHT22 and AM2302 sensors.

You should use sensorType value to match the sensor as follows:

| Sensor          | sensorType value |
| --------------- | :--------------: |
| DHT11           |        11        |
| DHT22 or AM2302 |        22        |

If the initialization succeeds when you can call the read function to obtain the latest readout from the sensor. Readout values contains a temperature and a humidity property.

### First Example

![example1](https://cloud.githubusercontent.com/assets/420851/20246902/1a03bafc-a9a8-11e6-8158-d68928b2e79f.png)

This sample queries a DHT11 sensor connected to the GPIO 4 and prints out the result on the console.

```javascript
var sensor = require("node-dht-sensor");

sensor.read(11, 4, function(err, temperature, humidity) {
  if (!err) {
    console.log(`temp: ${temperature}°C, humidity: ${humidity}%`);
  }
});
```

### Multiple Sensors Example

![example2](https://cloud.githubusercontent.com/assets/420851/20246914/554d72c4-a9a8-11e6-9162-ae51ecdf4212.png)

The following example shows a method for querying multiple sensors connected to the same Raspberry Pi. For this example, we have two sensors:

1. A DHT11 sensor connected to GPIO 17
2. High-resolution DHT22 sensor connected to GPIO 4

```javascript
var sensorLib = require("node-dht-sensor");

var app = {
  sensors: [
    {
      name: "Indoor",
      type: 11,
      pin: 17
    },
    {
      name: "Outdoor",
      type: 22,
      pin: 4
    }
  ],
  read: function() {
    for (var sensor in this.sensors) {
      var readout = sensorLib.read(
        this.sensors[sensor].type,
        this.sensors[sensor].pin
      );
      console.log(
        `[${this.sensors[sensor].name}] ` +
          `temperature: ${readout.temperature.toFixed(1)}°C, ` +
          `humidity: ${readout.humidity.toFixed(1)}%`
      );
    }
    setTimeout(function() {
      app.read();
    }, 2000);
  }
};

app.read();
```

### Promises API

Promises API provides an alternative `read` method that returns a Promise object rather than using a callback. The API is accessible via `require('node-dht-sensor').promises`.

```javascript
var sensor = require("node-dht-sensor").promises;

// You can use `initialize` and `setMaxTries` just like before
sensor.setMaxRetries(10);
sensor.initialize(22, 17);

// You can still use the synchronous version of `read`:
// var readout = sensor.readSync(22, 4);

sensor.read(22, 17).then(
  function(res) {
    console.log(
      `temp: ${res.temperature.toFixed(1)}°C, ` +
        `humidity: ${res.humidity.toFixed(1)}%`
    );
  },
  function(err) {
    console.error("Failed to read sensor data:", err);
  }
);
```

Using `async/await`:

```javascript
const sensor = require("node-dht-sensor").promises;

async function exec() {
  try {
    const res = await sensor.read(22, 4);
    console.log(
      `temp: ${res.temperature.toFixed(1)}°C, ` +
        `humidity: ${res.humidity.toFixed(1)}%`
    );
  } catch (err) {
    console.error("Failed to read sensor data:", err);
  }
}

exec();
```

### Test mode

A _test mode_ of operation is available since version `0.2.0`. In this mode of operation, the library does not communicate with the sensor hardware via the **GPIO** but instead it returns a pre-configured readout value. You can use the test mode during development without the need to have an actual sensor connected.

To enable the _test mode_, fake values must be defined at initialization. In the example below we specify fixed values for temperature equal to 21&deg;C and humidity equal to 60%.

```javascript
sensor.initialize({
  test: {
    fake: {
      temperature: 21,
      humidity: 60
    }
  }
});
```

After initialization, we can call the `read` method as usual.

```javascript
sensor.read(22, 4, function(err, temperature, humidity) {
  if (!err) {
    console.log(
      `temp: ${temperature.toFixed(1)}°C, ` +
        `humidity: ${humidity.toFixed(1)}%`
    );
  }
});
```

And the result will always be the configured readout value defined at initialization.

```shell session
$ node examples/fake-test.js
temp: 21.0°C, humidity: 60.0%
$ node examples/fake-test.js
temp: 21.0°C, humidity: 60.0%
```

You can find a complete source code example in [examples/fake-test.js](https://github.com/momenso/node-dht-sensor/blob/master/examples/fake-test.js).

### Reference for building from source

Standard node-gyp commands are used to build the module. So, just make sure you have node and node-gyp as well as the Broadcom library to build the project.

1. In case, you don't have node-gyp, install it first:

   ```shell session
   $ sudo npm install -g node-gyp
   $ sudo update-alternatives --install /usr/bin/node-gyp node-gyp /opt/node-v10.15.3-linux-armv7l/bin/node-gyp 1
   ```

2. Generate the configuration files

   ```shell session
   $ node-gyp configure
   ```

3. Build the component
   ```shell session
   $ node-gyp build
   ```

### Tracing and Debugging

Verbose output from the module can be enabled by by specifying the `--dht_verbose=true` flag when installing the node via npm.

```shell session
$ npm install node-dht-sensor --dht_verbose=true
```

if you are interested in enabling trace when building directly from source you can enable the `-Ddht_verbose` flag when running node-gyp configure.

```shell session
$ node-gyp configure -- -Ddht_verbose=true
```

### Appendix A: Quick Node.js installation guide

There are many ways you can get Node.js installed on your Raspberry Pi. Here is just one of way you can do it.

```shell session
$ wget https://nodejs.org/dist/v10.15.3/node-v10.15.3-linux-armv7l.tar.xz
$ tar xvfJ node-v10.15.3-linux-armv7l.tar.xz
$ sudo mv node-v10.15.3-linux-armv7l /opt
$ sudo update-alternatives --install /usr/bin/node node /opt/node-v10.15.3-linux-armv7l/bin/node 1
$ sudo update-alternatives --set node /opt/node-v10.15.3-linux-armv7l/bin/node
$ sudo update-alternatives --install /usr/bin/npm npm /opt/node-v10.15.3-linux-armv7l/bin/npm 1
```

Please note that you may have to use armv6l instead of arm7l if you have an early Raspberry Pi model.

### References

[1]: Node.js download - https://nodejs.org/en/download/

[2]: BCM2835 - http://www.airspayce.com/mikem/bcm2835/

[3]: Node.js native addon build tool - https://github.com/TooTallNate/node-gyp

[4]: GPIO: Raspbery Pi Models A and B - https://www.raspberrypi.org/documentation/usage/gpio/
