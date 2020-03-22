//
// Created by dear on 2020/3/21.
//
#include <nevent/IoBuf.h>

using namespace sn;

int getRand(int start, int end) {
    double rate = (double) random() / RAND_MAX;
    return (int) ((end - start) * rate) + start;
}

int main() {

    IoBuf buf;

    for (int i = 0; i < 100000000; ++i) {

        char *bs;
        size_t len;
        buf.writePtr(&bs, &len);
        CHECK(len > 0);
        size_t wLen = getRand(1, 300);
        if (wLen >= 299) {
            LOG(INFO) << wLen;

        }
        if (wLen > len) {
            wLen = len;
        }
        buf.addSize(wLen);
        IoBuf w;
        auto len1 = getRand(1, buf.getSize());
        buf.popInto(w, len1);
        w.clear(getRand(1, len1));
    }


    return 0;
}
