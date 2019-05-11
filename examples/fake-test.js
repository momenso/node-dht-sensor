var sensor = require("../lib");

sensor.initialize({
  test: {
    fake: {
      temperature: 21,
      humidity: 60
    }
  }
});

sensor.read(22, 4, function(err, temperature, humidity) {
  if (!err) {
    console.log(
      "temp: " +
        temperature.toFixed(1) +
        "°C, " +
        "humidity: " +
        humidity.toFixed(1) +
        "%"
    );
  }
});
