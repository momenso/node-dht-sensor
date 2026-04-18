# node-dht-sensor

A simple node.js module for reading temperature and relative humidity using a compatible DHT sensor.

![](https://github.com/momenso/node-dht-sensor/workflows/Node.js%20CI/badge.svg)
[![npm](https://img.shields.io/npm/v/node-dht-sensor.svg?label=npm%20package)](https://www.npmjs.com/package/node-dht-sensor)
[![npm](https://img.shields.io/npm/dm/node-dht-sensor.svg)](https://www.npmjs.com/package/node-dht-sensor)
[![LICENSE](https://img.shields.io/github/license/momenso/node-dht-sensor.svg)](https://github.com/momenso/node-dht-sensor/blob/master/LICENSE)

## Installation & Hardware Requirements

For most standard Linux systems and older Raspberry Pi models (Pi 1 through 4), you can install the module directly via npm:

```sh
npm install node-dht-sensor
```

### Modern Raspberry Pi (Pi 5 / Debian 12+) — libgpiod

When running `node-dht-sensor` on a modern architecture like the Raspberry Pi 5, the legacy BCM2835 library is physically incompatible with the new RP1 southbridge chip. You **must** compile the module using `libgpiod`.

**`libgpiod` v2 is the highly preferred library.** The v2 API allows this module to use an `OPEN_DRAIN` configuration, enabling fast, user-space polling of the sensor.

Before installing the module, you must install the `libgpiod` development headers and `pkg-config`. Our build system uses `pkg-config` to auto-detect whether your OS provides `libgpiod` v1 or v2 at build time and compiles the correct C++ code paths automatically.

For Debian-based systems (like Raspberry Pi OS):

```sh
sudo apt-get update
sudo apt-get install -y libgpiod-dev pkg-config
```

Then, explicitly tell npm to use the libgpiod API during installation:

```sh
npm install node-dht-sensor --use_libgpiod=true
```

### Older Raspberry Pi (Pi 1 through Pi 4) — BCM2835

If you omit the `--use_libgpiod=true` flag, `node-dht-sensor` defaults to using the direct-memory BCM2835 library. This bypasses the Linux kernel entirely for nanosecond-level read speeds and is highly recommended for older boards where the CPU manages the GPIO pins directly.

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

sensor.read(11, 4, function (err, temperature, humidity) {
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
      pin: 17,
    },
    {
      name: "Outdoor",
      type: 22,
      pin: 4,
    },
  ],
  read: function () {
    for (var sensor in this.sensors) {
      var readout = sensorLib.read(
        this.sensors[sensor].type,
        this.sensors[sensor].pin,
      );
      console.log(
        `[${this.sensors[sensor].name}] ` +
          `temperature: ${readout.temperature.toFixed(1)}°C, ` +
          `humidity: ${readout.humidity.toFixed(1)}%`,
      );
    }
    setTimeout(function () {
      app.read();
    }, 2000);
  },
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
  function (res) {
    console.log(
      `temp: ${res.temperature.toFixed(1)}°C, ` +
        `humidity: ${res.humidity.toFixed(1)}%`,
    );
  },
  function (err) {
    console.error("Failed to read sensor data:", err);
  },
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
        `humidity: ${res.humidity.toFixed(1)}%`,
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
      humidity: 60,
    },
  },
});
```

After initialization, we can call the `read` method as usual.

```javascript
sensor.read(22, 4, function (err, temperature, humidity) {
  if (!err) {
    console.log(
      `temp: ${temperature.toFixed(1)}°C, ` +
        `humidity: ${humidity.toFixed(1)}%`,
    );
  }
});
```

And the result will always be the configured readout value defined at initialization.

```sh
node examples/fake-test.js
temp: 21.0°C, humidity: 60.0%
node examples/fake-test.js
temp: 21.0°C, humidity: 60.0%
```

You can find a complete source code example in [examples/fake-test.js](https://github.com/momenso/node-dht-sensor/blob/master/examples/fake-test.js).

## Manual Compilation & Debugging

Standard node-gyp commands are used to build the module. So, just make sure you have `node` and `node-gyp`, and the appropriate C++ libraries (`bcm2835` or `libgpiod-dev`) to build the project.

1. In case you don't have node-gyp, install it first:

   ```sh
   sudo npm install -g node-gyp
   ```

2. To configure and build the component manually with libgpiod auto-detection enabled:

   ```sh
   node-gyp configure --use_libgpiod=true
   node-gyp build
   ```

## Tracing and Debugging

Verbose output from the module can be enabled by specifying the `--dht_verbose=true` flag when installing the node via npm.

```sh
npm install node-dht-sensor --dht_verbose=true
```

If you are interested in enabling trace when building directly from source you can enable the `-Ddht_verbose` flag when running node-gyp configure.

```sh
node-gyp configure -- -Ddht_verbose=true
```

## Appendix A: Quick Node.js installation guide

There are many ways you can get Node.js installed on your Raspberry Pi. For modern installations (including Raspberry Pi 5), the officially recommended approach is using the NodeSource package repository.

```sh
curl -fsSL [https://deb.nodesource.com/setup_20.x](https://deb.nodesource.com/setup_20.x) | sudo -E bash -
sudo apt-get install -y nodejs
```

_(This automatically installs the correct architecture build (e.g., `arm64`/`aarch64` for Pi 5 or `armhf` for older models) and includes npm)._

## References

[1]: Node.js download - https://nodejs.org/en/download/

[2]: BCM2835 - http://www.airspayce.com/mikem/bcm2835/

[3]: libgpiod - https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/

[4]: Node.js native addon build tool - https://github.com/TooTallNate/node-gyp
