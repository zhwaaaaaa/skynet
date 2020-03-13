//
// Created by dear on 2020/2/10.
//
#include "shake_hands_handler.h"
#include "ResponseHandler.h"

namespace sn {

    ShakeHandsHandler::ShakeHandsHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), serviceSize(0) {
    }

    int ShakeHandsHandler::onMessage(IoBuf &buf) {
        vector<string> servs;
        uint32_t size = buf.getSize();
        serviceSize = CONVERT_VAL_16(buf.read<uint16_t>(5));
        uint32_t offset = 7;
        for (int i = 0; i < serviceSize; ++i) {
            string s;

            auto len = buf.readUint8(offset);
            if (offset + len >= size) {
                LOG(WARNING) << "decode shake hands decode size " << (offset + len) << " greater than size " << size;
                return -1;
            }
            s.resize(len);
            buf.copyIntoPtr(len, s.data(), offset + 1);
            offset += len + 1;
            servs.push_back(std::move(s));
        }
        if (offset != size) {
            LOG(WARNING) << "decode size expected " << size << " but got " << offset;
            return -1;
        }
        doShakeHands(servs);
        return 0;
    }

    ClientShakeHandsHandler::ClientShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ClientShakeHandsHandler::doShakeHands(vector<string> &services) {
        LOG(INFO) << services[0];
        return 0;
    }


    ServerShakeHandsHandler::ServerShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ServerShakeHandsHandler::doShakeHands(vector<string> &services) {
        LOG(INFO) << services[0];
        return 0;
    }
}
