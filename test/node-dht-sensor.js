var assert = require("chai").assert;
var sensor = require("../lib");
var psensor = require("../lib").promises;

const SENSOR_TYPE = parseInt(process.env.SENSOR_TYPE || 11, 10);
const GPIO_PIN = parseInt(process.env.GPIO_PIN || 4, 10);

describe("Initialize", () => {
  describe("Initialize mock sensor", () => {
    it("should initialize to provide fake readouts", () => {
      sensor.initialize({
        test: {
          fake: {
            temperature: 42,
            humidity: 72
          }
        }
      });
    });
    it("should fail when initializing fake readout without temperature value", () => {
      assert.throws(
        () => {
          sensor.initialize({
            test: {
              fake: {
                humidity: 60
              }
            }
          });
        },
        Error,
        "Test mode: temperature value must be defined for a fake"
      );
    });
    it("should fail when initializing fake readout without humidity value", () => {
      assert.throws(
        () => {
          sensor.initialize({
            test: {
              fake: {
                temperature: 17
              }
            }
          });
        },
        Error,
        "Test mode: humidity value must be defined for a fake"
      );
    });
  });
  describe("Sensor type and GPIO pin", () => {
    it("should throw error if sensor type is not supported", () => {
      assert.throws(
        () => {
          sensor.initialize(0, 0);
        },
        TypeError,
        "Specified sensor type is not supported"
      );
    });
    it("should initialize with valid sensor type and GPIO pin", () => {
      sensor.initialize(SENSOR_TYPE, GPIO_PIN);
    });
  });
  describe("With invalid arguments", () => {
    it("should fail when no arguments are provided", () => {
      assert.throws(sensor.initialize, TypeError, "Wrong number of arguments");
    });
    it("should fail when sensor type is not numeric", () => {
      assert.throws(
        () => {
          sensor.initialize("11", 0);
        },
        TypeError,
        "Invalid arguments"
      );
    });
    it("should fail when GPIO pin is not numeric", () => {
      assert.throws(
        () => {
          sensor.initialize(SENSOR_TYPE, "4");
        },
        TypeError,
        "Invalid arguments"
      );
    });
    it("should fail when maxRetries is not numeric", () => {
      assert.throws(
        () => {
          sensor.initialize(SENSOR_TYPE, GPIO_PIN, "abc");
        },
        TypeError,
        "Invalid maxRetries parameter"
      );
    });
  });
});

describe("Set max retries", () => {
  it("should fail if an argument is not provided", () => {
    assert.throws(sensor.setMaxRetries, TypeError, "Wrong number of arguments");
  });
  it("should set max retries", () => {
    sensor.setMaxRetries(5);
  });
});

describe("Read sensor", () => {
  describe("Synchronously", () => {
    it("should return a readout when no parameter is provided", () => {
      sensor.initialize(SENSOR_TYPE, GPIO_PIN);
      var readout = sensor.read();

      assert.isObject(readout);
      assert.hasAllKeys(readout, [
        "temperature",
        "humidity",
        "isValid",
        "errors"
      ]);
    });
    it("should return a readout when sensor type and GPIO pin are provided", () => {
      var readout = sensor.read(SENSOR_TYPE, GPIO_PIN);
      assert.isObject(readout);
      assert.hasAllKeys(readout, [
        "temperature",
        "humidity",
        "isValid",
        "errors"
      ]);
    });
    it("should fail when invalid sensor type is specified", () => {
      assert.throws(
        () => {
          sensor.read(3, GPIO_PIN);
        },
        TypeError,
        "specified sensor type is invalid"
      );
    });
  });
  describe("Asynchronously", () => {
    it("should obtain temperature and humidity", done => {
      sensor.read(SENSOR_TYPE, GPIO_PIN, (err, temperature, humidity) => {
        assert.notExists(err);
        assert.isNumber(temperature);
        assert.isNumber(humidity);
        done();
      });
    });
    it("should fail when invalid sensor type is specified", done => {
      sensor.read(3, GPIO_PIN, err => {
        assert.exists(err);
        assert.throws(
          () => {
            assert.ifError(err, "sensor type is invalid");
          },
          Error,
          "sensor type is invalid"
        );
        done();
      });
    });
  });
  describe("Asynchronously (promise)", () => {
    it("should obtain temperature and humidity", async () => {
      const { temperature, humidity } = await psensor.read(
        SENSOR_TYPE,
        GPIO_PIN
      );
      assert.isNumber(temperature);
      assert.isNumber(humidity);
    });
    it("should fail when invalid sensor type is specified", async () => {
      try {
        await psensor.read(3, GPIO_PIN);
        assert.fail("should have failed");
      } catch (err) {
        assert.throws(
          () => {
            assert.ifError(err, "sensor type is invalid");
          },
          Error,
          "sensor type is invalid"
        );
      }
    });
  });
  describe("With invalid arguments", () => {
    it("should fail if too many arguments are provided", () => {
      assert.throws(
        () => {
          sensor.read(1, 2, 3, 4);
        },
        TypeError,
        "invalid number of arguments"
      );
    });
  });
});
