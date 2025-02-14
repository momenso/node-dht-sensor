const sensor = require("../lib");

function read() {
  sensor.read(22, 17, (err, temperature, humidity) => {
    console.log(err, temperature, humidity);
    setTimeout(read, 2000);
  });
}

read();
