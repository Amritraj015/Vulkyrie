#pragma once

#include "defines.h"

class SystemsManager
{
private:
    static b8 initialized;

public:
    static void initialize_systems();
    static void terminate_systems();
};