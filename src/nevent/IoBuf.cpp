//
// Created by dear on 3/5/20.
//

#include "IoBuf.h"
#include <cstdlib>

namespace sn {
    BlockPool::BlockPool(int maxSize) noexcept: maxSize(maxSize), current(0), head(nullptr) {
    }

    static thread_local BlockPool pool;

    Block *BlockPool::get() {
        Block *b;
        if (!pool.current) {
            b = static_cast<Block *>(malloc(BLOCK_SIZE));
        } else {
            b = pool.head;
            pool.head = b->next;
            --pool.current;
        }

        b->ref = 1;
        b->next = nullptr;
        return b;
    }

    void BlockPool::recycle(Block *b) {
        CHECK(b->ref == 0);
        if (pool.current >= pool.maxSize) {
            free(b);
        } else {
            b->next = pool.head;
            pool.head = b;
            ++pool.current;
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

}

