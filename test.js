// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var sensorLib = require('./build/Release/node-dht-sensor');

var sensor = {
  initialize: function() {
    return sensorLib.initialize(22, 4);
  },

  read: function() {
    var readout = sensorLib.read();
    if (readout.humidity != 0 && readout.temperature != 0)
      console.log('Temperature: '+readout.temperature.toFixed(1)+'C, humidity: '+readout.humidity.toFixed(1)+'%');
    setTimeout(function() {
      sensor.read();
    }, 1000);
  }
};

if (sensor.initialize()) {
  sensor.read();
} else {
  console.warn('Failed to initialize sensor');
}
