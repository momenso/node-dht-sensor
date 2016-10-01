// Module node-dht-sensor demo
// Read asynchronously from the sensor

var sensor = require('../build/Release/node_dht_sensor');
var count = 0;
var iid = setInterval(function() {
  if (++count > 10) {
    clearInterval(iid);
  }

  sensor.read(22, 4, function(err, temperature, humidity) {
    if (err) {
      console.warn('' + err);
    } else {
      console.log('temp: ' + temperature.toFixed(1) + '°C, ' +
        'humidity: ' + humidity.toFixed(1) + '%'
      );
    }
  });

}, 2500);
