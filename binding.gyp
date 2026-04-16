{
  "targets": [
    {
      "variables": {
        "dht_verbose%": "false",
        "use_libgpiod%" : "false",
        "libgpiod_version%": "<!(node -p \"try { require('child_process').execSync('pkg-config --modversion libgpiod').toString().trim().split('.')[0] >= 2 ? 'GPIOD_V2' : 'GPIOD_V1' } catch(e) { 'GPIOD_V1' }\")"
      },
      "target_name": "node_dht_sensor",
      "sources": [
        "src/bcm2835/bcm2835.c",
        "src/node-dht-sensor.cpp",
        "src/dht-sensor.cpp",
        "src/util.cpp",
        "src/abstract-gpio.cpp"
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
          "defines": [ 
            "USE_LIBGPIOD",
            "<(libgpiod_version)"
          ],
          "libraries": [
            "-lgpiod"
          ]
        }]
      ]
    }
  ]
}