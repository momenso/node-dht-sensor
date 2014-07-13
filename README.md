# node-dht-sensor

This node.js module supports querying air temperature and relative humidity from a compatible DHT sensor.

## Installation
    $ npm install node-dht-sensor

## Usage

This module uses the [BCM2835](http://www.airspayce.com/mikem/bcm2835/) library that requires access to 
/open/mem. Because of this, you will typically run node with admin privileges.

The first step is initializing the sensor by specifying the sensor type and which GPIO pin the sensor is connected. It should work for DHT11, DHT22 and AM2302 sensors. If the initialization succeeds when you can call the read function to obtain the latest readout from the sensor. Readout values contains a temperature and a humidity property.

### Example

This sample queries the AM2302 sensor connected to the GPIO 4 every 1.5 seconds and displays the result on the console. 

```javascript
var sensorLib = require('node-dht-sensor');

var sensor = {
  initialize: function() {
    return sensorLib.initialize(22, 4);
  },
  read: function() {
    var readout = sensorLib.read();
    console.log('Temperature: '+readout.temperature.toFixed(2)+'C, humidity: '+readout.humidity.toFixed(2)+'%');
    setTimeout(function() {
      sensor.read();
    }, 1500);
  }
};

if (sensor.initialize()) {
  sensor.read();
} else {
  console.warn('Failed to initialize sensor');
}
```

## Reference for building from source

Standard node-gyp commands are used to build the module.

1. Generate the configuration files
```
node-gyp configure
```
2. Build the component
```
node-gyp build
```

### Verbose output

Verbose output from the module can be enabled by defining ```VERBOSE``` during the module compilation. For example, this can be enabled via the binging,gyp file:

```javascript
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
