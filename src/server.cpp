#define SPARSEHASH
#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <msgpack.hpp>
#include <netinet/in.h>

#ifdef SPARSEHASH
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;
#endif

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <signal.h>
#include <netdb.h>
#include <cmath>

#include <city.h>

#include <tr1/functional>
#include <tr1/unordered_map>
#include "msgpack/type/tr1/unordered_map.hpp"
#include <iostream>
#include <fstream>

#include <server.hpp>

// Uses:

using namespace std;

using std::cout;
using std::endl;
using tr1::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS

// Global Variables:

#ifdef SPARSEHASH
#define HASHTYPE dense_hash_map<string, string, hash<string>, eqstring>
#else
#define HASHTYPE tr1::unordered_map<string, string>
#endif
HASHTYPE datastore;
boost::mutex datastore_mtx;

uint64 hostKey;

// Functions:

void error(const char *msg)
{
  cout << "ERROR: " << msg << endl;
  exit(1);
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


struct StringToIntSerializer {
  bool operator()(FILE* fp, const std::pair<string, string>& value) const {

    msgpack::sbuffer sbuf;

    sbuf.write("\n", 1);

    msgpack::packer<msgpack::sbuffer> pk2(&sbuf);
    pk2.pack_map(3);
    pk2.pack(string("cmd"));
    pk2.pack(string("SET"));
    pk2.pack(string("key"));
    pk2.pack(string(value.first));
    pk2.pack(string("val"));
    pk2.pack(value.second);

    //cout << "DATA: " << sbuf.data() << endl;

    //msgpack::sbuffer sbuf;  // simple buffer
    //msgpack::pack(&sbuf, value);
    /*
    // Write the key.  We ignore endianness for this example.
    if (fwrite(&value.first, sizeof(value.first), 1, fp) != 1)
      return false;
    // Write the value.
    assert(value.second.length() <= 255);   // we only support writing small strings
    const unsigned char size = value.second.length();
    if (fwrite(&size, 1, 1, fp) != 1)
      return false;
    if (fwrite(value.second.data(), size, 1, fp) != 1)
      return false;*/
    if (fwrite(sbuf.data(), sbuf.size() + 1, 1, fp) != 1)
      return false;
    return true;
  }
  // This doesn't work.
  bool operator()(FILE* fp, std::pair<string, string>* value) const {
    char* line;
    size_t len = 0;
    if (!getline(&line, &len, fp)) {
      return false;
    }

    // Read the key.  Note the need for const_cast to get around
    // the fact hash_map keys are always const.

    /*if (fread(&value->first, sizeof(value->first), 1, fp) != 1)
      return false;
    // Read the value.
    unsigned char size;    // all strings are <= 255 chars long
    if (fread(&size, 1, 1, fp) != 1)
      return false;
    char* buf = new char[size];
    if (fread(buf, size, 1, fp) != 1) {
      delete[] buf;
      return false;
    }
    new(&value->second) string(buf, size);
    delete[] buf;*/
    return true;
  }
};
#define DBFILE "010dht.db"
int load() {
  std::ifstream infile(DBFILE);
  if (infile) {
  //if (FILE* fp = fopen(DBFILE, "r")) {
    cout << "Loading db file..." << endl;

    // Skip first
    bool first = true;

    string line;
    while (std::getline(infile, line)) {
      if (first) {
        first = false;
      } else {
        // TODO: load data
        //cout << line << ", RESP: " << processCmd(line.c_str()) << endl;
      }
    }

    //fclose(fp);
    infile.close();
    cout << "Done!" << endl;
  }
  return 0;
}
int save() {
  cout << "Saving..." << endl;
  FILE* fp = fopen(DBFILE, "w");
  datastore_mtx.lock();
#ifdef SPARSEHASH
  datastore.serialize(StringToIntSerializer(), fp);
#else
  // TODO: tr1 serialize
#endif
  datastore_mtx.unlock();
  fclose(fp);
  cout << "Done!" << endl;
  return 0;
}
int sockfd;
void sighandler(int sig)
{
  cout<< "Signal " << sig << " caught..." << endl;
  int yes=1;
  //char yes='1'; // use this under Solaris

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("setsockopt");
      exit(1);
  }
  close(sockfd);
  save();
  exit(0);
}


int main(int argc, char *argv[]) {
  int newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  if (argc < 2) {
    portno = 8002;
  } else {
    portno = atoi(argv[1]);
  }

  hostKey = CityHash64CXX(boost::lexical_cast<string>(portno));

  cout << "HOST KEY: " << hostKey << endl;

#ifdef SPARSEHASH
  datastore.set_empty_key(string(""));
#endif

  load();

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
   error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
       sizeof(serv_addr)) < 0)
       error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  cout << "Server listening on: " << portno << endl;
  while (1) {
    newsockfd = accept(sockfd,
        (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");
    boost::thread(dostuff, newsockfd);
  }
  close(sockfd);
  return 0;
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
  while(true) {
    int n;
    char buffer[256];

    bzero(buffer,256);
    n = read(sock,buffer,255);
    if (n < 0) {
      error("ERROR reading from socket");
      break;
    } else if(n > 0) {
      string resp = processCmd(sock, buffer);
      n = write(sock, resp.c_str(), resp.size());
      if (n < 0) error("ERROR writing to socket");
#ifdef DEBUG
      cout << "RESP " << resp << endl;
#endif
    } else {
      cout << "CONNECTION LOST" << endl;
      break;
    }
  }
}

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


vector<Node> knownNodes;


int addNodes(vector<Node> nodes) {
  int added = 0;
  for(vector<Node>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
    // Check if node is us.
    bool found = false;
    for(vector<Node>::iterator it2 = knownNodes.begin(); it2 != knownNodes.end(); ++it2) {
      if (it->equals(*it2)) {
        found = true;
        break;
      }
    }
    if (!found) {
      cout << "Adding node: " << *it << endl;
      added++;
      it->introduceMyself();
    }
  }
  return added;
}

// Finds node with the closest key.
string passOrOperateCmd(const char* buffer, tr1::unordered_map<string, msgpack::object> map) {
  string cmd, key;

  try {
    map["cmd"].convert(&cmd);
    map["key"].convert(&key);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  uint64 hashedKey = CityHash64CXX(key);

  Node* best = NULL;

  uint64 bestDist = hashDistance(hashedKey, hostKey);

  for(vector<Node>::iterator it = knownNodes.begin(); it != knownNodes.end(); ++it) {
    uint64 itDist = hashDistance(hashedKey, it->key);
    if (itDist < bestDist) {
      bestDist = itDist;
      best = &*it;
    }
  }

  if (best != NULL) {
    cout << "Forwarding request to: " << *best << endl;
    return best->sendMessage(buffer);
  }

  string resp;

  if (cmd == "SET") {
    string val;
    try {
      map["val"].convert(&val);
    } catch (msgpack::type_error) {
      return "ERR BAD CMD";
    }
    if (key.size() == 0) {
      resp = "ERR INVALID KEY";
    } else {
      datastore_mtx.lock();
      datastore[key] = val;
      datastore_mtx.unlock();
      resp = "OK";
    }
  } else if (cmd == "GET") {
    datastore_mtx.lock();
    resp = datastore[key];
    datastore_mtx.unlock();
    if (resp.size() == 0) resp = "ERR NOT FOUND";
  }
  return resp;
}

string processCmd(int sock, const char* buffer) {
  string resp = "ERR NO RESP";
  // Deserialize the serialized data.
  msgpack::unpacked msg;
  msgpack::unpack(&msg, buffer, 256);
  msgpack::object obj = msg.get();

  cout << obj << endl;

  tr1::unordered_map<string, msgpack::object> map;

  string cmd;
  try {
    obj.convert(&map);
    map["cmd"].convert(&cmd);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  if (cmd == "NODEADD") {
    Node node;

    try {
      map["host"].convert(&node.host);
      map["port"].convert(&node.port);
    } catch (msgpack::type_error) {
      return "ERR BAD CMD";
    }

    return node.introduceMyself();

  } else if (cmd == "NODEINTRO") {

    // Read knownNodes

    vector<Node> remoteKnownNodes;

    Node node;

    node.host = getSockIP(sock);

    try {
      map["knownNodes"].convert(&remoteKnownNodes);
      map["port"].convert(&node.port);
      map["key"].convert(&node.key);
    } catch (msgpack::type_error) {
      return "ERR BAD CMD";
    }

    remoteKnownNodes.push_back(node);

    addNodes(remoteKnownNodes);

    // Respond with our info

    msgpack::sbuffer buffer2;
    msgpack::packer<msgpack::sbuffer> pk2(&buffer2);
    pk2.pack_map(2);
    pk2.pack(string("key"));
    pk2.pack(hostKey);

    pk2.pack(string("knownNodes"));
    pk2.pack(knownNodes);
    resp = string(buffer2.data());
  } else if (cmd == "NODEKEYS") {
    vector<string> keys;
    for(HASHTYPE::iterator it = datastore.begin(); it != datastore.end(); ++it) {
      keys.push_back(it->first);
    }
    msgpack::sbuffer buf;
    msgpack::packer<msgpack::sbuffer> pk(&buf);
    pk.pack(keys);
    resp = buf.data();
  } else if (cmd == "NODEDUMPMAP") {
    tr1::unordered_map<string, string> map;
    for(HASHTYPE::iterator it = datastore.begin(); it != datastore.end(); ++it) {
      map[it->first] = it->second;
    }
    msgpack::sbuffer buf;
    msgpack::packer<msgpack::sbuffer> pk(&buf);
    pk.pack(map);
    resp = buf.data();
  } else if (cmd == "NODES") {
    msgpack::sbuffer buf;
    msgpack::packer<msgpack::sbuffer> pk(&buf);
    pk.pack(knownNodes);
    resp = string(buf.data());
  } else if (cmd == "SET" || cmd == "GET") {
    resp = passOrOperateCmd(buffer, map);
  } else {
    resp = "ERR UNKNOWN CMD";
  }
  return resp;
}
