//
// Created by dear on 20-1-31.
//

#include "reactor.h"

namespace sn {

    void Reactor::callWakeUpFunc() {
        lock_guard g(funcLock);
        while (!funcQue.empty()) {
            const FuncWrap &evt = funcQue.front();
            evt.func(evt.param);
            funcQue.pop();
        }
    }

}


