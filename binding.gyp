{
  'targets': [
    {
      "variables": {
        "dht_verbose%": "false"
      },
      "target_name": "node_dht_sensor",
      "sources": [ "node-dht-sensor.cpp", "dht-sensor.cpp" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "libraries": [ "-lbcm2835" ],
      "conditions": [
        ["OS=='linux'", {
          "cflags": [ "-std=c++11" ],
          "include_dirs+": "/usr/local/lib/libbcm2835.a",
          "sources": ["node-dht-sensor.cpp", "dht-sensor.cpp" ]
        }],
        ["dht_verbose=='true'", {
          "defines": [ "VERBOSE" ]
        }]
      ]
    }
  ]
}
