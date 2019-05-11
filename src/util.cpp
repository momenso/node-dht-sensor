#include "util.h"

Napi::Value objectGetDefined(Napi::Object object, const std::string &key)
{
  if (!object.Has(key))
  {
    return Napi::Value();
  }

  Napi::Value value = object.Get(key);
  if (value.IsNull() || value.IsUndefined())
  {
    return Napi::Value();
  }

  return value;
}
