#include <sys/socket.h>
#include <netinet/in.h>
#include <util.hpp>
#include <arpa/inet.h>
#include <city.h>
#include <msgpack.hpp>
#include "msgpack/type/tr1/unordered_map.hpp"

#include <boost/lexical_cast.hpp>

string getSockIP(int s) {
  socklen_t len;
  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN];
  int port;

  len = sizeof addr;
  getpeername(s, (struct sockaddr*)&addr, &len);

  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in *)&addr;
      port = ntohs(s->sin_port);
      inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
  } else { // AF_INET6
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
      port = ntohs(s->sin6_port);
      inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
  }
  return ipstr;
}

uint64 CityHash64CXX(string str) {
  return CityHash64(str.c_str(), str.size());
}
uint64 hashDistance(uint64 a, uint64 b) {
  uint64 d = a - b;
  uint64 e = UINT_MAX - (a - b);
  if (d < e) {
    return d;
  } else {
    return e;
  }
}


void packSetCmd(msgpack::sbuffer *sbuf, string key, string val) {
  msgpack::packer<msgpack::sbuffer> pk2(sbuf);
  pk2.pack_map(3);
  pk2.pack(string("cmd"));
  pk2.pack(string("SET"));
  pk2.pack(string("key"));
  pk2.pack(string(key));
  pk2.pack(string("val"));
  pk2.pack(val);
}
