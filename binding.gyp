{
  "variables": {
    "use_libgpiod%": "false",
    "dht_verbose%": "false",
    "libgpiod_version": "<!(node -e \"try { var v = require('child_process').execSync('pkg-config --modversion libgpiod', {stdio: 'pipe'}).toString().trim(); console.log(parseInt(v.split('.')[0], 10) >= 2 ? '2' : '1'); } catch(e) { console.log('1'); }\")"
  },
  "targets": [
    {
      "target_name": "node_dht_sensor",
      "sources": [
        "src/bcm2835/bcm2835.c",
        "src/dht-sensor.cpp",
        "src/node-dht-sensor.cpp",
        "src/util.cpp",
        "src/abstract-gpio.cpp"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "cflags": [
        "-O3",
        "-Wno-unused-variable",
        "-Wno-unused-function"
      ],
      "conditions": [
        [
          "dht_verbose=='true'",
          {
            "defines": [
              "VERBOSE"
            ]
          }
        ],
        [
          "use_libgpiod=='true'",
          {
            "defines": [
              "USE_LIBGPIOD",
              "GPIOD_V<(libgpiod_version)"
            ],
            "libraries": [
              "-lgpiod"
            ],
            "sources!": [
              "src/bcm2835/bcm2835.c"
            ]
          }
        ]
      ]
    }
  ]
}