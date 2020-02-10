//
// Created by dear on 2020/2/5.
//

#include "client.h"
#include <memory>


namespace sn {

    ServiceKeeper *Client::getByService(const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            return nullptr;
        }

        return iterator->second.get();
    }

    void Client::enableService(uint32_t channelId, const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator != serviceMap.end()) {
            iterator->second->care(channelId);
        } else {
            char *servCpy = static_cast<char *>(malloc(serv->len + 1));
            memcpy(servCpy, serv->buf, serv->len);
            servCpy[serv->len] = '\0';
            string_view x(servCpy, serv->len);

            auto y = std::make_shared<ServiceKeeper>(x);
            y->care(channelId);
            serviceMap.insert(make_pair(x, y));
        }
    }

    void Client::disableService(uint32_t channelId, const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            LOG(WARNING) << "Not found service enabled:" << key;
            return;
        }

        if (iterator->second->noCare(channelId) && !iterator->second->count()) {
            LOG(INFO) << "disable not need service:" << key;
            // 需要删除malloc的key
            const char *keyVal = iterator->first.data();
            serviceMap.erase(iterator);
            free((void *) keyVal);
        }
    }
}
