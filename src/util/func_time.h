//
// Created by dear on 20-1-31.
//

#ifndef SKYNET_TIME_H
#define SKYNET_TIME_H

#include <zconf.h>
#include <ctime>
#include <sys/time.h>

namespace sn {
    typedef void (*ActionFunc)(void *param);

    struct FuncWrap {
        void *param;
        ActionFunc func;

        FuncWrap(void *param, ActionFunc func) : param(param), func(func) {
        }
    };

    long int currentTime();

}


#endif //SKYNET_TIME_UTIL_H
