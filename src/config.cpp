#include "utils/hashmap.cpp"

#define KEY_LEFT 0x0064
#define KEY_RIGHT 0x0066
#define KEY_UP 0x0065
#define KEY_DOWN 0x0067

class Config {
  public:
    Hashtable<char, string> key_mappings = {
      {' ', "space"},
      {'a', "left"},
      {'d', "right"},
      {'w', "up"},
      {'s', "down"}
    };
    Hashtable<int, string> skey_mappings = {
      {KEY_LEFT, "left"},
      {KEY_RIGHT, "right"},
      {KEY_UP, "up"},
      {KEY_DOWN, "down"}
    };
};
