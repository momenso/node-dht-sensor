var sensor = require("../build/Release/node_dht_sensor");

var promises = {
  initialize: sensor.initialize,
  setMaxRetries: sensor.setMaxRetries,
  readSync(type, pin) {
    return sensor.read(type, pin);
  },
  read(type, pin) {
    return new Promise(function(resolve, reject) {
      sensor.read(type, pin, function(err, temperature, humidity) {
        if (err) {
          reject(err);
        } else {
          resolve({ temperature, humidity });
        }
      });
    });
  }
};

module.exports = {
  initialize: sensor.initialize,
  read: sensor.read,
  setMaxRetries: sensor.setMaxRetries,
  promises
};
