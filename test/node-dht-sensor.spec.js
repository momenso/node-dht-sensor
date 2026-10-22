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
            humidity: 72,
          },
        },
      });
    });

    it("should fail when initializing fake readout without temperature value", () => {
      expect(() => {
        sensor.initialize({
          test: {
            fake: {
              humidity: 60,
            },
          },
        });
      }).toThrow("Test mode: temperature value must be defined for a fake");
    });

    it("should fail when initializing fake readout without humidity value", () => {
      expect(() => {
        sensor.initialize({
          test: {
            fake: {
              temperature: 17,
            },
          },
        });
      }).toThrow("Test mode: humidity value must be defined for a fake");
    });
  });

  describe("Sensor type and GPIO pin", () => {
    it("should throw error if sensor type is not supported", () => {
      expect(() => sensor.initialize(0, 0)).toThrow(
        "Specified sensor type is not supported",
      );
    });

    it("should initialize with valid sensor type and GPIO pin", () => {
      sensor.initialize(SENSOR_TYPE, GPIO_PIN);
    });
  });
  describe("With invalid arguments", () => {
    it("should fail when no arguments are provided", () => {
      expect(sensor.initialize).toThrow("Wrong number of arguments");
    });

    it("should fail when sensor type is not numeric", () => {
      expect(() => {
        sensor.initialize("11", 0);
      }).toThrow("Invalid arguments");
    });

    it("should fail when GPIO pin is not numeric", () => {
      expect(() => {
        sensor.initialize(SENSOR_TYPE, "4");
      }).toThrow("Invalid arguments");
    });

    it("should fail when maxRetries is not numeric", () => {
      expect(() => {
        sensor.initialize(SENSOR_TYPE, GPIO_PIN, "abc");
      }).toThrow("Invalid maxRetries parameter");
    });
  });
});

describe("Set max retries", () => {
  it("should fail if an argument is not provided", () => {
    expect(sensor.setMaxRetries).toThrow("Wrong number of arguments");
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

      expect(readout).toEqual(expect.any(Object));
      expect(Object.keys(readout).sort()).toEqual(
        ["errors", "humidity", "isValid", "temperature"].sort(),
      );
    });

    it("should return a readout when sensor type and GPIO pin are provided", () => {
      var readout = sensor.read(SENSOR_TYPE, GPIO_PIN);
      expect(readout).toEqual(expect.any(Object));
      expect(Object.keys(readout).sort()).toEqual(
        ["errors", "humidity", "isValid", "temperature"].sort(),
      );
    });

    it("should fail when invalid sensor type is specified", () => {
      expect(() => sensor.read(3, GPIO_PIN)).toThrow(
        "specified sensor type is invalid",
      );
    });
  });

  describe("Asynchronously", () => {
    it("should obtain temperature and humidity", (done) => {
      sensor.read(SENSOR_TYPE, GPIO_PIN, (err, temperature, humidity) => {
        expect(err).toBeFalsy();
        expect(typeof temperature).toBe("number");
        expect(typeof humidity).toBe("number");
        done();
      });
    });

    it("should fail when invalid sensor type is specified", (done) => {
      sensor.read(3, GPIO_PIN, (err) => {
        expect(err).toBeTruthy();
        expect(err.message).toMatch(/sensor type is invalid/);
        done();
      });
    });
  });

  describe("Asynchronously (promise)", () => {
    it("should obtain temperature and humidity", async () => {
      const { temperature, humidity } = await psensor.read(
        SENSOR_TYPE,
        GPIO_PIN,
      );
      expect(typeof temperature).toBe("number");
      expect(typeof humidity).toBe("number");
    });

    it("should fail when invalid sensor type is specified", async () => {
      await expect(psensor.read(3, GPIO_PIN)).rejects.toThrow(
        "sensor type is invalid",
      );
    });
  });

  describe("With invalid arguments", () => {
    it("should fail if too many arguments are provided", () => {
      expect(() => sensor.read(1, 2, 3, 4)).toThrow(
        "invalid number of arguments",
      );
    });
  });
});

describe("Wrongly-typed arguments", () => {
  beforeAll(() => {
    sensor.initialize({ test: { fake: { temperature: 21, humidity: 60 } } });
  });

  const sym = Symbol("x");
  const cb = () => {};

  it.each([
    ["setMaxRetries(Symbol)", () => sensor.setMaxRetries(sym)],
    ["read(Symbol, pin)", () => sensor.read(sym, 4)],
    ["read(type, Symbol)", () => sensor.read(11, sym)],
    ["read(Symbol, pin, cb)", () => sensor.read(sym, 4, cb)],
    ["read(type, Symbol, cb)", () => sensor.read(11, sym, cb)],
    ["read(type, pin, 'not a function')", () => sensor.read(11, 4, "cb")],
    ["read(type, pin, undefined)", () => sensor.read(11, 4, undefined)],
    ["read('11', pin) numeric string", () => sensor.read("11", 4)],
    ["setMaxRetries('3') numeric string", () => sensor.setMaxRetries("3")],
  ])("%s throws a catchable TypeError", (_name, call) => {
    let err;
    try {
      call();
    } catch (e) {
      err = e;
    }

    expect(err).toBeDefined();
    expect(err.name).toBe("TypeError");
    expect(err.message).toBe("Invalid arguments");
  });
});
