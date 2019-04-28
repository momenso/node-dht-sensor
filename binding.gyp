{
  'targets': [
    {
      "variables": {
        "dht_verbose%": "false"
      },
      "target_name": "node_dht_sensor",
      "sources": [ 
        "src/bcm2835/bcm2835.c", 
        "src/node-dht-sensor.cpp", 
        "src/dht-sensor.cpp" 
      ],
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
