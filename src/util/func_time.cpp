//
// Created by dear on 20-1-31.
//

#include "func_time.h"

long int ::sn::currentTime() {
    timeval tv{0};
    if (gettimeofday(&tv, nullptr)) {
        return -1;
    }
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
