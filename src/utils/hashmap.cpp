#include <iostream>
#include <unordered_map>
#include <initializer_list>

using namespace std;

template <typename KeyType, typename ValueType>
class Hashtable {
  unordered_map<KeyType, ValueType> htmap;

  public:
    // Default constructor
    Hashtable() = default;

    // Constructor with initializer list
    Hashtable(initializer_list<pair<KeyType, ValueType>> initList) {
      for (const auto& entry : initList) {
        htmap[entry.first] = entry.second;
      }
    }

    void put(const KeyType& key, const ValueType& value) {
      htmap[key] = value;
    }

    ValueType get(const KeyType& key) {
      return htmap[key];
    }
};
