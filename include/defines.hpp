#define DEBUG
#define SPARSEHASH

#include <string>

using namespace std;

#ifdef SPARSEHASH
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;
#endif

#ifndef EQSTR
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
#define EQSTR
#endif

#ifdef SPARSEHASH
#define HASHTYPE dense_hash_map<string, string, hash<string>, eqstring>
#else
#define HASHTYPE tr1::unordered_map<string, string>
#endif

using tr1::hash;
