// explicit sensor read test
var sensor = require('../build/Release/node_dht_sensor');
var usage = 'USAGE: node sync-explicit.js [sensorType] [gpioPin] <repeats>\n' +
    '    sensorType:\n' +
    '         11: For DHT11 sensor.\n' +
    '         22: For DHT22 or AM2302 sensors.\n\n' +
    '    gpipPin:\n' +
    '         Pin number where the sensor is physically connected to.\n\n' +
    '    repeats:\n' +
    '         How many times the read operation will be performed, default: 10\n';

if (process.argv.length < 4) {
    console.warn(usage);
    process.exit(1);
}

var sensorType = parseInt(process.argv[2], 10);
var gpioPin = parseInt(process.argv[3], 10);
var repeats = parseInt(process.argv[4] || '10', 10);
var count = 0;

var iid = setInterval(function() {
  if (++count >= repeats) {
    clearInterval(iid);
  }
  var start = new Date().getTime();
  var readout = sensor.read(sensorType, gpioPin);
  var end = new Date().getTime();
  console.log(`temperature: ${readout.temperature.toFixed(1)}°C, ` +
              `humidity: ${readout.humidity.toFixed(1)}%, ` +
              `valid: ${readout.isValid}, ` +
              `errors: ${readout.errors}, ` +
              `time: ${end - start}ms`);
}, 2500);
