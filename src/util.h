#ifndef _UTIL_H
#define _UTIL_H

#include <napi.h>
#include <string>

// Return empty Napi::Value if `object` doesn't have `key`
// or it is null or undefined. Otherwise return the value in `key`.
Napi::Value objectGetDefined(Napi::Object object, const std::string &key);

#endif
