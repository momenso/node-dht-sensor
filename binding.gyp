{
  "targets": [
    {
      "variables": {
        "dht_verbose%": "false",
        "use_libgpiod%" : "false"
      },
      "target_name": "node_dht_sensor",
      "sources": [
        "src/bcm2835/bcm2835.c",
        "src/node-dht-sensor.cpp",
        "src/dht-sensor.cpp",
        "src/util.cpp",
        "src/abstract-gpio.cpp",
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      "conditions": [
        ["dht_verbose=='true'", {
          "defines": [ "VERBOSE" ]
        }],
        ["use_libgpiod=='true'", {
          "defines": [ "USE_LIBGPIOD" ],
          'libraries': [
            '-lgpiod'
          ]
        }]
      ]
    }
  ]
}
