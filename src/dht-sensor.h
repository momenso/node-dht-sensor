#ifndef _DHT_H
#define _DHT_H

int initialize();
unsigned long long getTime();
long readDHT(int type, int pin, float &temperature, float &humidity);

#endif
