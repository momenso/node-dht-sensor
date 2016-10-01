// Module node-dht-sensor demo
// Read asynchronously from the sensor

var sensor = require('../build/Release/node_dht_sensor');
var Process = require('process');

var usage = 'USAGE: node async-explicit.js [sensorType] [gpioPin] <repeats>\n' +
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

var sensorType = parseInt(Process.argv[2], 10);
var gpioPin = parseInt(Process.argv[3], 10);
var repeats = parseInt(Process.argv[4] || '10', 10);
var count = 0;

var iid = setInterval(function() {
  if (++count >= repeats) {
    clearInterval(iid);
  }

  sensor.read(sensorType, gpioPin, function(err, temperature, humidity) {
    if (err) {
      console.warn('' + err);
    } else {
      console.log('temp: ' + temperature.toFixed(1) + '°C, ' +
        'humidity: ' + humidity.toFixed(1) + '%'
      );
    }
  });

}, 2500);
