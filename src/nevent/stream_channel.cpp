//
// Created by dear on 2020/2/4.
//

#include "stream_channel.h"

static thread_local uint32_t _id;

uint32_t sn::generateChannelId() {
    return _id++;
}
