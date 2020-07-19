var sensor = require("../lib").promises;

sensor
  .read(22, 17)
  .then(
    function(res) {
      console.log(
        `temp: ${res.temperature.toFixed(1)}°C, ` +
          `humidity: ${res.humidity.toFixed(1)}%`
      );
    },
    function(err) {
      console.error("Failed to read sensor data:", err);
    }
  )
  .catch(e => console.error("Error: " + e));
console.log("done");
