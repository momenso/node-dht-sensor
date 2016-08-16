// Module node-dht-sensor demo
// Reads relative air humidity from DHT sensor

var fs = require('fs');
var sensorLib = require('./build/Release/node_dht_sensor');

var sensor = {
  initialize: function() {
    this.totalReads = 0;
	this.totalFailures = 0;
    return sensorLib.initialize(22, 4);
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
	this.totalFailures += readout.errors;
    if (this.totalReads < 10) {
      setTimeout(function() {
        sensor.read();
      }, 2000);
    } else {
		console.log('error rate: '+this.totalFailures+'/'+this.totalReads+': '+
			((this.totalFailures*100)/(this.totalReads+this.totalFailures)).toFixed(2)+'%');
	}
  }
};

if (sensor.initialize()) {
  sensor.read();
} else {
  console.warn('Failed to initialize sensor');
}
