//
// Created by dear on 2020/2/10.
//

#include "channel_keeper.h"
#include <glog/logging.h>

namespace sn {
    ChannelKeeper::ChannelKeeper(Channel *channel) : channelPtr(channel) {
        LOG(INFO) << "ChannelKeeper(channel)" << channel;
    }

    ChannelKeeper::~ChannelKeeper() {
        LOG(INFO) << "~ChannelKeeper()" << channelPtr.get();
    }
}
