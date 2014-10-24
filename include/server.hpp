#include <string>
#include <node.hpp>

using namespace std;

void dostuff(int);
string processCmd(int, const char*);
int addNodes(vector<Node>);

void packSetCmd(msgpack::sbuffer*, string, string);

uint64 extern hostKey;


