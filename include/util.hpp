#include <string>
#include <city.h>
#include <msgpack.h>
#include "msgpack/type/tr1/unordered_map.hpp"

using namespace std;

string getSockIP(int s);

uint64 CityHash64CXX(string str);
uint64 hashDistance(uint64 a, uint64 b);

void packSetCmd(msgpack::sbuffer*, string, string);
