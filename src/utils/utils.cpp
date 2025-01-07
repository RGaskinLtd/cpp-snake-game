#include <iostream>
using namespace std;

const char * intToStr(int data) {
  string strData = to_string(data);
  char* temp = new char[strData.length() + 1];
  strcpy(temp, strData.c_str());
  return temp;
}
