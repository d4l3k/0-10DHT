/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <msgpack.hpp>
#include <netinet/in.h>
#include <sparsehash/dense_hash_map>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <signal.h>
#include <netdb.h>

#include <city.h>

#include <tr1/functional>
#include <tr1/unordered_map>
#include "msgpack/type/tr1/unordered_map.hpp"
#include <iostream>
#include <fstream>

#define DEBUG

// Uses:

using namespace std;

using google::dense_hash_map;      // namespace where class lives by default
using std::cout;
using std::endl;
using tr1::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS

// Data Definitions:

class Node {
  public:
    string host;
    int port;
    uint64 key;
    string sendMessage(string);
    template <typename Packer>
    void msgpack_pack(Packer&) const;
    void msgpack_unpack(msgpack::object);
    bool equals(Node);
    string introduceMyself();
    friend std::ostream& operator<< (std::ostream&, Node const&);
};

// Headers:

void dostuff(int); /* function prototype */
string processCmd(const char*);
int addNodes(vector<Node>);

// Functions:

void error(const char *msg)
{
  cout << "ERROR: " << msg << endl;
  exit(1);
}

uint64 CityHash64CXX(string str) {
  return CityHash64(str.c_str(), str.size());
}

struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};

dense_hash_map<const char*, string, hash<const char*>, eqstr> datastore;
boost::mutex datastore_mtx;           // mutex for critical section
struct StringToIntSerializer {
  bool operator()(FILE* fp, const std::pair<const char*, string>& value) const {

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
  bool operator()(FILE* fp, std::pair<const char*, string>* value) const {
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

    /*char * line = NULL;
    size_t len = 0;
    ssize_t read;


    while ((read = getline(&line, &len, fp)) != -1) {
    }
    */

    string line;
    while (std::getline(infile, line)) {
      if (first) {
        first = false;
      } else {
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
  datastore_mtx.lock();
  FILE* fp = fopen(DBFILE, "w");
  datastore.serialize(StringToIntSerializer(), fp);
  datastore_mtx.unlock();
  fclose(fp);
  cout << "Done!" << endl;
  return 0;
}
int sockfd;
void sighandler(int sig)
{
  cout<< "Signal " << sig << " caught..." << endl;
  close(sockfd);
  int yes=1;
  //char yes='1'; // use this under Solaris

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("setsockopt");
      exit(1);
  }
  save();
  exit(0);
}

uint64 hostKey;

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

  datastore.set_empty_key("");
  datastore.resize(100);

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
    /*pid = fork();
    if (pid < 0)
      error("ERROR on fork");
    if (pid == 0)  {
      close(sockfd);
      dostuff(newsockfd);
      exit(0);
    }
    else close(newsockfd);*/
    boost::thread(dostuff, newsockfd);
  } /* end of while */
  close(sockfd);
  return 0; /* we never get here */
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
      string resp = processCmd(buffer);
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




vector<Node> knownNodes;

template <typename Packer>
void Node::msgpack_pack(Packer& pk) const {
  pk.pack_map(3);
  pk.pack(string("host"));
  pk.pack(host);
  pk.pack(string("port"));
  pk.pack(port);
  pk.pack(string("key"));
  pk.pack(key);
}
void Node::msgpack_unpack(msgpack::object o)
{
  std::tr1::unordered_map<string, msgpack::object> map;
  try {
    o.convert(&map);
  } catch (msgpack::type_error) {
  }
  map.find("host")->second.convert(&host);
  map.find("port")->second.convert(&port);
  map.find("key")->second.convert(&key);
}

bool Node::equals(Node n) {
  return key == n.key;
}

std::ostream& operator<< (std::ostream& o, Node const& n)
{
  return o << "{ " << n.key << ", " << n.host << ":" << n.port << " }";
}

string Node::sendMessage(string message) {
  int sd, ret;
  struct sockaddr_in server;
  //struct in_addr ipv4addr;
  struct hostent *hp;

  sd = socket(AF_INET,SOCK_STREAM,0);
  server.sin_family = AF_INET;

  //inet_pton(AF_INET, host.c_str(), &ipv4addr);
  //hp = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
  hp = gethostbyname(host.c_str());

  bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
  server.sin_port = htons(port);

  if (connect(sd, (const sockaddr *)&server, sizeof(server)) != 0) {
    return "ERR BAD ADDRESS";
  }
  send(sd, message.c_str(), message.size(), 0);

  char buffer[256];

  bzero(buffer,256);
  read(sd,buffer,255);
  return string(buffer);
}
string Node::introduceMyself() {
  msgpack::sbuffer buffer;
  msgpack::packer<msgpack::sbuffer> pk2(&buffer);
  pk2.pack_map(3);
  pk2.pack(string("cmd"));
  pk2.pack(string("NODEINTRO"));
  pk2.pack(string("hash"));
  pk2.pack(hostKey);

  pk2.pack(string("knownNodes"));
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, knownNodes);

  pk2.pack(knownNodes);

  string hash = sendMessage(string(buffer.data()));
  msgpack::unpacked msg2;
  msgpack::unpack(&msg2, hash.c_str(), hash.size());
  std::tr1::unordered_map<string, msgpack::object> map;
  msgpack::object obj3 = msg2.get();
  try {
    obj3.convert(&map);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }
  map.find("hash")->second.convert(&key);

  bool found = false;
  for(vector<Node>::iterator it2 = knownNodes.begin(); it2 != knownNodes.end(); ++it2) {
    if (it2->equals(*this)) {
      found = true;
      break;
    }
  }
  // Check trying to add self.
  if (!found && key != hostKey) {
    knownNodes.push_back(*this);
  }

  vector<Node> remoteKnownNodes;

  try {
    map.find("knownNodes")->second.convert(&remoteKnownNodes);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  addNodes(remoteKnownNodes);

  return "OK";
}

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



string processCmd(const char* buffer) {
  string resp = "ERR NO RESP";
  // Deserialize the serialized data.
  msgpack::unpacked msg;
  msgpack::unpack(&msg, buffer, 256);
  msgpack::object obj = msg.get();

  cout << obj << endl;

  std::tr1::unordered_map<string, msgpack::object> map;
  try {
    obj.convert(&map);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  string cmd;
  map.find("cmd")->second.convert(&cmd);
  if (cmd == "NODEADD") {
    Node node;

    map.find("host")->second.convert(&node.host);
    map.find("port")->second.convert(&node.port);

    return node.introduceMyself();

  } else if (cmd == "NODEINTRO") {

    // Read knownNodes

    vector<Node> remoteKnownNodes;

    try {
      map.find("knownNodes")->second.convert(&remoteKnownNodes);
    } catch (msgpack::type_error) {
      return "ERR BAD CMD";
    }

    addNodes(remoteKnownNodes);

    // Respond with our info

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> pk2(&buffer);
    pk2.pack_map(2);
    pk2.pack(string("hash"));
    pk2.pack(hostKey);

    pk2.pack(string("knownNodes"));
    pk2.pack(knownNodes);
    resp = string(buffer.data());


  } else if (cmd == "SET" || cmd == "GET") {
    string key;
    map.find("key")->second.convert(&key);
    uint64 hashedKey = CityHash64CXX(key);
    if (cmd == "SET") {
      string val;
      map.find("val")->second.convert(&val);
      if (key.size() == 0) {
        resp = "ERR INVALID KEY";
      } else {
        datastore_mtx.lock();
        datastore[key.c_str()] = val;
        datastore_mtx.unlock();
        resp = "OK";
      }
    } else if (cmd == "GET") {
      datastore_mtx.lock();
      resp = datastore[key.c_str()];
      datastore_mtx.unlock();
      if (resp.size() == 0) resp = "ERR NOT FOUND";
    }
  } else {
    resp = "ERR UNKNOWN CMD";
  }
  return resp;
}
