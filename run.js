// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var sensorLib = require('./build/Release/node-dht-sensor');

var sensor = {
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
