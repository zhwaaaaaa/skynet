//
// Created by dear on 3/13/20.
//

#include <glog/logging.h>
#include <nevent/io_error.h>
#include "ZkNamingServer.h"

namespace sn {


    void ZkNamingServer::watcherZookeeper(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx) {
    }


    void ZkNamingServer::watcherService(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx) {
        ZkNamingServer *server = static_cast<ZkNamingServer *>(watcherCtx);
        lock_guard guard(server->subscribeLock);
        auto iterator = server->subscribeMap.find(string(path));
        if (iterator == server->subscribeMap.end()) {
            return;
        }
        shared_ptr<WatchContext> ctx = iterator->second;
        String_vector val{};
        int r;
        if ((r = zoo_wget_children(zh, ctx->path.c_str(), watcherService, watcherCtx, &val)) == ZOK) {
            ctx->val.clear();
            for (int i = 0; i < val.count; ++i) {
                ctx->val.emplace_back(val.data[i]);
            }
            ctx->func(string_view(ctx->service), ctx->val, ctx->param);
        } else {
            LOG(WARNING) << "error lookup " << zerror(r);
        }
    }

    ZkNamingServer::ZkNamingServer(const ZkConfig &conf) {
        handle = nullptr;
        config = conf;
        config.zkNamespace = "/";
        if (!conf.zkNamespace.empty()) {
            config.zkNamespace.append(conf.zkNamespace);
        } else {
            config.zkNamespace.append("skynet");
        }
        zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
        handle = zookeeper_init(config.ipHosts.data(), watcherZookeeper, config.recvTimeout,
                                &clientId, this, 0);

        if (!handle) {
            CHECK(handle) << "zookeeper连接失败:" << config.ipHosts;
        }
        int r;
        Stat stat;
        if ((r = zoo_exists(handle, config.zkNamespace.c_str(), 0, &stat)) == ZNONODE) {
            r = zoo_create(handle, config.zkNamespace.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, 0,
                           nullptr, 0);
        }
        CHECK(r == ZOK) << "ZkNamingServer 启动失败:" << zerror(r);
    }

    unique_ptr<vector<string>>
    ZkNamingServer::subscribe(const std::string_view &serviceName, SubscribeFunc func, void *param) {
        string path = config.zkNamespace;
        path.append("/");
        String_vector val{};
        path.append(serviceName);
        lock_guard guard(subscribeLock);
        subscribeMap.insert_or_assign(path, make_shared<WatchContext>(func, param, path, serviceName));
        Stat stat{};
        int r = zoo_exists(handle, path.c_str(), 0, &stat);

        if (r == ZNONODE) {
            r = zoo_create(handle, path.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, 0,
                           nullptr, 0);
        }

        unique_ptr<vector<string>> ret(new vector<string>);
        if (r != ZOK) {
            return ret;
        }

        if ((r = zoo_wget_children(handle, path.c_str(), watcherService, this, &val)) == ZOK) {
            for (int i = 0; i < val.count; ++i) {
                ret->emplace_back(val.data[i]);
            }
            LOG(INFO) << "subscribe service " << serviceName;
        } else {
            LOG(WARNING) << "subscribe " << serviceName << " fail:" << zerror(r);
        }
        return ret;
    }


    void ZkNamingServer::unsubscribe(const string_view &serviceName) {
        string path = config.zkNamespace;
        path.append("/");
        path.append(serviceName);
        lock_guard guard(subscribeLock);
        auto i = subscribeMap.find(path);
        if (i != subscribeMap.end()) {
            LOG(INFO) << "unsubscribe service " << serviceName;
            subscribeMap.erase(i);
        }
    }

    void ZkNamingServer::registerService(const string_view &serviceName) {
        try {
            createNode(serviceName);
        } catch (IoError &err) {
            LOG(WARNING) << "register " << serviceName << " failed " << err.what();
        }
    }


    void ZkNamingServer::createNode(const string_view &serviceName) const {
        string path = config.zkNamespace;
        path.append("/");
        path.append(serviceName);
        Stat stat{};
        int i = zoo_exists(handle, path.c_str(), 0, &stat);
        if (i == ZNONODE) {
            i = zoo_create(handle, path.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, 0,
                           nullptr, 0);
        }

        if (i != ZOK) {
            throw IoError(i, zerror(i));
        }
        path.append("/");
        path.append(config.registerStr);

        i = zoo_exists(handle, path.c_str(), 0, &stat);
        if (i == ZOK) {
            zoo_delete(handle, path.c_str(), -1);
        }

        i = zoo_create(handle, path.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL,
                       nullptr, 0);
        if (i != ZOK) {
            throw IoError(i, zerror(i));
        }
    }

    void ZkNamingServer::unregisterService(const string_view &service) {
        try {
            removeNode(service);
        } catch (IoError &err) {
            LOG(WARNING) << "register " << service << " failed " << err.what();
        }
    }

    unique_ptr<vector<string>> ZkNamingServer::lookup(const string_view &service) {
        String_vector val{};
        unique_ptr<vector<string>> ret(new vector<string>);
        string path = config.zkNamespace;
        path.append("/");
        path.append(service);
        int r;
        if ((r = zoo_get_children(handle, path.c_str(), 0, &val)) == ZOK) {
            for (int i = 0; i < val.count; ++i) {
                char *serv = val.data[i];
                ret->push_back(serv);
            }
        } else {
            LOG(WARNING) << "error lookup " << zerror(r);
        }

        return ret;
    }

    ZkNamingServer::~ZkNamingServer() {
        zookeeper_close(handle);
    }

    void ZkNamingServer::removeNode(const string_view &service) {
        string path = config.zkNamespace;
        path.append("/");
        path.append(service);
        path.append("/");
        path.append(config.registerStr);
        int i = zoo_delete(handle, path.c_str(), -1);
        if (i == ZNONODE) {
            LOG(INFO) << "delete not exists node " << path;
        }
        if (i != ZOK) {
            throw IoError(i, zerror(i));
        }
    }


}

