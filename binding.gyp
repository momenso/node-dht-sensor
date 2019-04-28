{
  'targets': [
    {
      "variables": {
        "dht_verbose%": "false"
      },
      "target_name": "node_dht_sensor",
      "sources": [ "bcm2835/bcm2835.c", "node-dht-sensor.cpp", "dht-sensor.cpp" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "conditions": [
        ["dht_verbose=='true'", {
          "defines": [ "VERBOSE" ]
        }]
      ]
    }
  ]
}
