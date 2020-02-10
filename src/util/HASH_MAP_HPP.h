//
// Created by dear on 20-1-29.
//

#ifndef SKYNET_HASH_MAP_HPP_H
#define SKYNET_HASH_MAP_HPP_H

#include <string>
// Definition in butil/third_party/superfasthash/superfasthash.c. (Third-party
// code did not come with its own header file, so declaring the function here.)
// Note: This algorithm is also in Blink under Source/wtf/StringHasher.h.
extern "C" uint32_t SuperFastHash(const char *data, int len);

#if defined(_STLPORT_VERSION)
#include <hash_map>
#include <hash_set>
using std::hash;
using std::hash_map;
using std::hash_set;
#else // not using STLPORT


#ifdef __GNUC__
#if __GNUC__ >= 3

#include <ext/hash_map>
#include <ext/hash_set>

#define SN_HASH_NAMESPACE __gnu_cxx

using SN_HASH_NAMESPACE::hash_map;
using SN_HASH_NAMESPACE::hash;
using SN_HASH_NAMESPACE::hash_set;
#else // GCC 2.x
#define SN_HASH_NAMESPACE std

#include <hash_map>
#include <hash_set>
namespace std {
    struct hash<std::string> {
        size_t operator()(const std::string& s) const {
            unsigned long __h = 0;
            for (unsigned i = 0;i < s.size();++i)
                __h ^= (( __h << 5) + (__h >> 2) + s[i]);
            return size_t(__h);
        }
    };
};
using std::hash_map;
using std::hash_set;
using std::hash;
#endif // end GCC >= 3
#elif defined(_MSC_VER) && ((_MSC_VER >= 1300) || defined(__INTEL_COMPILER))
// we only support MSVC7+ and Intel C++ 8.0
#define SN_HASH_NAMESPACE stdext
#include <hash_map>
#include <hash_set>
        namespace stdext {
            inline size_t hash_value(const std::string& s) {
                unsigned long __h = 0;
                for (unsigned i = 0;i < s.size();++i)
                    __h ^= (( __h << 5) + (__h >> 2) + s[i]);
                return size_t(__h);
            }
        }
        using std::hash_map; // _MSC_EXTENSIONS, though DEPRECATED
        using std::hash_set;
#else
#error unknown compiler
#endif //GCC or MSVC7+
#endif // end STLPORT

namespace SN_HASH_NAMESPACE {
    template<>
    struct hash<std::string> {
        size_t operator()(const std::string &s) const {
            return ::SuperFastHash(s.data(), s.length());
        }

    };
}
namespace SN_HASH_NAMESPACE {
    template<>
    struct hash<std::string_view> {
        size_t operator()(const std::string_view &s) const {
            return ::SuperFastHash(s.data(), s.length());
        }

    };
}
#endif //SKYNET_HASH_MAP_HPP_H

