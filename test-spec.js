// Module node-dht-sensor demo
// Reads from two DHT sensors

var sensorLib = require('./build/Release/node-dht-sensor');

var sensor = {
  sensors: [ 
    { name: 'Indoor', type: 11, pin: 27 },
    { name: 'Outdoor', type: 22, pin: 17 }
  ],

  read: function() {
    for (var i in this.sensors) {
      var readout = sensorLib.readSpec(this.sensors[i].type, 
          this.sensors[i].pin);
      console.log(this.sensors[i].name+': '+
                  readout.temperature.toFixed(1)+'C, '+
                  readout.humidity.toFixed(1)+'%');
    }
    setTimeout(function() {
      sensor.read();
    }, 2000);
  }
};

sensor.read();
