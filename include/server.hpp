#include <string>
#include <node.hpp>

using namespace std;

void dostuff(int);
string processCmd(int, const char*);
int addNodes(vector<Node>);

uint64 extern hostKey;


struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};
struct eqstring
{
  bool operator()(string s1, string s2) const
  {
    return (s1 == s2) || (s1.compare(s2) == 0);
  }
};
