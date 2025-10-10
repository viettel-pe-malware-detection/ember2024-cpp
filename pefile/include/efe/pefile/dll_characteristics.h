#ifndef PEFILE_DLL_CHARACTERISTICS_INCLUDED
#define PEFILE_DLL_CHARACTERISTICS_INCLUDED

#include <cstdint>

using dll_characteristic_t = uint16_t;

class DLLCharacteristic {
public:
    #define ADD(value, name) \
        static constexpr dll_characteristic_t const name = value; \
    
    
    #include "dll_characteristics_declarations.h"

    #undef ADD
};

#endif // PEFILE_DLL_CHARACTERISTICS_INCLUDED
