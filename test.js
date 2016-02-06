// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var fs = require('fs');
var sensorLib = require('./build/Release/node_dht-sensor');

var sensor = {
  initialize: function() {
    this.totalReads = 0;
    return sensorLib.initialize(22, 17);
  },

  read: function() {
    var readout = sensorLib.read();
    this.totalReads++;
    console.log('Temperature: '+readout.temperature.toFixed(1)+'C, humidity: '+readout.humidity.toFixed(1)+'%'+
                ', valid: '+readout.isValid+
                ', errors: '+readout.errors);
    fs.appendFile('log.csv', 
      new Date().getTime()+','+readout.temperature+','+readout.humidity+',"'+(readout.checksum ? 'Ok' : 'Failed')+'",'+readout.errors+'\n', 
      function (err) { });
    if (this.totalReads < 300) {
      setTimeout(function() {
        sensor.read();
      }, 500);
    }
  }
};

if (sensor.initialize()) {
  sensor.read();
} else {
  console.warn('Failed to initialize sensor');
}
