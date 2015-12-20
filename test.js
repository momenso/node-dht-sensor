// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var fs = require('fs');
var sensorLib = require('./build/Release/node-dht-sensor');
var Process = require('process');

var usage = 'Usage: node test.js [SENSOR_TYPE] [GPIO_PIN]\
           \n    SENSOR_TYPE:\
	   \n      * DHT11 -> 11\
	   \n      * DHT22, AM2302 -> 22';

if (Process.argv.length != 4) {
    console.warn(usage);

    Process.exit(1);
}

var sensor = {
  initialize: function() {
    this.totalReads = 0;
    return sensorLib.initialize(parseInt(Process.argv[2], 10),
		                parseInt(Process.argv[3], 10));
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
