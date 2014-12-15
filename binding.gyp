{
  'targets': [
    {
      'target_name': 'node-dht-sensor',
      'sources': [ 'node-dht-sensor.cpp' ],
      'libraries': [ '-lwiringPi' ],
      'conditions': [ 
        ['OS=="linux"', {
          'include_dirs+': '/usr/local/lib/libbcm2835.a',
          'sources': ['node-dht-sensor.cpp'] 
        }]
      ]
    }
  ]
}
