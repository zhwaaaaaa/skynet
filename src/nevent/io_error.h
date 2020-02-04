//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_IO_ERROR_H
#define SKYNET_IO_ERROR_H

#include <exception>
#include <stdexcept>
#include <iostream>
#include <ostream>

namespace sn {
    using namespace std;

    struct IoError : public runtime_error {
        const int code;

        IoError(int code, const char *msg) : runtime_error(msg), code(code) {
        }

    };

    inline ostream &operator<<(ostream &out, const IoError &e) {
        return out << e.what() << ":" << uv_err_name(e.code);
    }
}


#endif //SKYNET_IO_ERROR_H
