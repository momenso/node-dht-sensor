// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var fs = require('fs');
var sensorLib = require('./build/Release/node_dht_sensor');
var Process = require('process');

var usage = 'USAGE: node test.js [sensorType] [gpioPin] <repeats>\n' +
            '    sensorType:\n' +
            '         11: For DHT11 sensor.\n' +
            '         22: For DHT22 or AM2302 sensors.\n\n' +
            '    gpipPin:\n' +
            '         Pin number where the sensor is physically connected to.\n\n' +
            '    repeats:\n' +
            '         How many times the read operation will be performed, default: 10\n';

if (Process.argv.length < 4) {
    console.warn(usage);
    Process.exit(1);
}

var sensor = {
  initialize: function() {
    this.totalReads = 0;
    this.totalFailures = 0;
    this.repeats = parseInt(Process.argv[4] || '10', 10);
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
      new Date().getTime()+','+readout.temperature+','+readout.humidity+',"'+(readout.isValid ? 'Ok' : 'Failed')+'",'+readout.errors+'\n',
      function (err) { });
    this.totalFailures += readout.errors;
    this.totalReads += readout.errors;
    if (this.totalReads <= this.repeats) {
      setTimeout(function() {
        sensor.read();
      }, 2500);
    } else {
		console.log('error rate: '+this.totalFailures+'/'+this.totalReads+': '+
			((this.totalFailures*100)/this.totalReads).toFixed(2)+'%');
	  }
  }
};

if (sensor.initialize()) {
  sensor.read();
} else {
  console.warn('Failed to initialize sensor');
}
