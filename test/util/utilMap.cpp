//
// Created by dear on 2020/2/15.
//

#include <util/HASH_MAP_HPP.h>
#include <iostream>

using namespace std;

int main() {
    hash_set<int> sets;
    sets.insert(1);
    sets.insert(1);
    cout << sets.size() << endl;
    sets.erase(1);
    cout << sets.size() << endl;
    sets.erase(1);
    cout << sets.size() << endl;

    hash_map<int, string> map;
    map.insert(make_pair(1, "111111"));

    for (const auto &pair:map) {
        cout << pair.first << "-" << pair.second << endl;
        map.erase(pair.first);
    }

    cout << sizeof(map.begin()) << endl;

}

