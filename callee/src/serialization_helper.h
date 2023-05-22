#ifndef SERIALIZATION_HELPER_H
#define SERIALIZATION_HELPER_H

#include <sstream>
#include <string>

template<typename T>
std::string seal_ser(const T& value)
{
    std::stringstream ss;
    value.save(ss);
    return ss.str();
}

template<typename T>
T seal_deser(const std::string& value, const seal::SEALContext &context)
{
    std::stringstream ss(value);
    T retval;
    retval.load(context, ss);
    return retval;
}

#endif
