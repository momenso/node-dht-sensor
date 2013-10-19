# node-dht-sensor

This node.js module supports querying air temperature and relative humidity from a compatible DHT sensor.

## Installation
    $ npm install node-dht-sensor

## Usage

This module uses the [BCM2835](http://www.airspayce.com/mikem/bcm2835/) library that requires access to 
/open/mem. Because of this, you will typically run node with admin privileges.

### Example

This sample queries the sensor every 1.5 seconds and displays the result on the console. 

```javascript var 
sensorLib = require('./build/Release/node-dht-sensor'); var sensor = {
  initialize: function() {
    return sensorLib.initialize(11, 4);
  },
  read: function() {
    var readout = sensorLib.read();
    console.log('Temperature: '+readout.temperature+'C, humidity: '+readout.humidity+'%');
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
