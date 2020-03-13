//
// Created by dear on 3/5/20.
//

#include "IoBuf.h"
#include <cstdlib>

namespace sn {
    BlockPool::BlockPool(int maxSize) noexcept:
            maxSize(maxSize), current(0), head(nullptr),
            totalMalloc(0), currentMalloc(0) {
    }

    static thread_local BlockPool pool;

    Block *BlockPool::get() {
        BlockPool &p = pool;
        Block *b;
        if (!p.current) {
            b = static_cast<Block *>(malloc(BLOCK_SIZE));
            ++p.totalMalloc;
            ++p.currentMalloc;
        } else {
            b = p.head;
            p.head = b->next;
            --p.current;
        }

        b->ref = 1;
        b->next = nullptr;
        return b;
    }

    void BlockPool::recycle(Block *b) {
        BlockPool &p = pool;
        CHECK(b->ref == 0);
        if (p.current >= p.maxSize) {
            --p.currentMalloc;
            free(b);
        } else {
            b->next = p.head;
            p.head = b;
            ++p.current;
        }

    }

    BlockPool::~BlockPool() {
        for (int i = current; i >= 0; --i) {
            Block *t = head;
            CHECK(t && t->ref == 0);
            head = t->next;
            free(t);
        }
    }

    void BlockPool::report() {
        BlockPool &p = pool;
        LOG(INFO) << "current=" << p.current << ",currentMalloc" << p.currentMalloc << ",totalMalloc" << p.totalMalloc;
    }

}

