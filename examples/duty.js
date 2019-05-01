var fs = require('fs');
var sensor = require('../build/Release/node_dht_sensor');

var usage = 'USAGE: node duty.js [sensorType] [gpioPin] <repeats>\n' +
    '    sensorType:\n' +
    '         11: For DHT11 sensor.\n' +
    '         22: For DHT22 or AM2302 sensors.\n\n' +
    '    gpipPin:\n' +
    '         Pin number where the sensor is physically connected to.\n\n';

if (process.argv.length < 4) {
    console.warn(usage);
    process.exit(1);
}

var sensorType = parseInt(process.argv[2], 10);
var gpioPin = parseInt(process.argv[3], 10);
var count = 0;
var start = 0;
var end = 0;

var iid = setInterval(function() {
  start = new Date().getTime();
  sensor.read(sensorType, gpioPin, function(err, temperature, humidity) {
  end = new Date().getTime();
    if (err) {
      console.warn('' + err);
    } else {
      var elapsed = end - start;
      console.log("temperature: %s°C, humidity: %s%%, time: %dms",
                temperature.toFixed(1), humidity.toFixed(1), end - start);
      fs.appendFile('log.csv',
                  new Date().getTime() + ',' +
                  temperature.toFixed(2) + ',' +
                  humidity.toFixed(2) + ',' +
                  elapsed + '\n',
                  function(err) {});
    }
  });
}, 1000); // 2500
