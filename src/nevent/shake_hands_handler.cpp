//
// Created by dear on 2020/2/10.
//
#include <event/server.h>
#include "shake_hands_handler.h"
#include "ResponseHandler.h"
#include "RequestHandler.h"

namespace sn {

    ShakeHandsHandler::ShakeHandsHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), serviceSize(0) {
    }

    int ShakeHandsHandler::onMessage(IoBuf &buf) {

        auto msgType = buf.readUint8();
        if (msgType != MT_CONSUMER_SH && msgType != MT_PROVIDER_SH) {
            return -1;
        }

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
        auto i = doShakeHands(servs);
        if (i < 0) {
            LOG(WARNING) << "shake hands error";
        }
        return i;
    }

    ClientShakeHandsHandler::ClientShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ClientShakeHandsHandler::doShakeHands(vector<string> &services) {
        auto pHandler = ch->replaceHandler(new ClientAppHandler(ch, services));
        CHECK(pHandler == this);
        delete this;
        return 0;
    }


    ServerShakeHandsHandler::ServerShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ServerShakeHandsHandler::doShakeHands(vector<string> &services) {
        auto pHandler = ch->replaceHandler(new ServerAppHandler(ch, services));
        CHECK(pHandler == this);
        delete this;
        return 0;
    }
}
