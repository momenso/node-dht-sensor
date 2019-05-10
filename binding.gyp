{
  "targets": [
    {
      "variables": {
        "dht_verbose%": "false",
        "banana_pro%": "false"
      },
      "target_name": "node_dht_sensor",
      "sources": [
        "src/node-dht-sensor.cpp",
        "src/dht-sensor.cpp",
        "src/util.cpp"
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
        ["banana_pro=='true'", {
          "sources": [
            "src/bcm2835_bp/bcm2835.c",
          ],
          "defines": [ "BANANA_PRO" ]
        }],
        ["banana_pro=='false'", {
          "sources": [
            "src/bcm2835/bcm2835.c",
          ]
        }]
      ]
    }
  ]
}
