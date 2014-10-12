#include <msgpack.h>
#include <string>

using namespace std;

uint64 extern hostKey;


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

int extern addNodes(vector<Node>);
vector<Node> extern knownNodes;

template <typename Packer>
void Node::msgpack_pack(Packer& pk) const {
  pk.pack_map(3);
  pk.pack(string("host"));
  pk.pack(this->host);
  pk.pack(string("port"));
  pk.pack(this->port);
  pk.pack(string("key"));
  pk.pack(this->key);
}
void Node::msgpack_unpack(msgpack::object o)
{
  std::tr1::unordered_map<string, msgpack::object> map;
  try {
    o.convert(&map);
    map["host"].convert(&host);
    map["port"].convert(&port);
    map["key"].convert(&key);
  } catch (msgpack::type_error) {
  }
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
  close(sd);
  return string(buffer);
}
string Node::introduceMyself() {

  bool found = false;
  for(vector<Node>::iterator it2 = knownNodes.begin(); it2 != knownNodes.end(); ++it2) {
    if (it2->equals(*this)) {
      found = true;
      break;
    }
  }

  // Check trying to add self.
  if (found) {
    return "ERR ALREADY ADDED";
  } else if (key == hostKey) {
    return "ERR SELF";
  }

  msgpack::sbuffer buffer;
  msgpack::packer<msgpack::sbuffer> pk2(&buffer);
  pk2.pack_map(4);
  pk2.pack(string("cmd"));
  pk2.pack(string("NODEINTRO"));
  pk2.pack(string("port"));
  pk2.pack(port);
  pk2.pack(string("key"));
  pk2.pack(hostKey);
  pk2.pack(string("knownNodes"));
  pk2.pack(knownNodes);

  string hash = sendMessage(string(buffer.data()));

  msgpack::unpacked msg2;
  msgpack::unpack(&msg2, hash.c_str(), hash.size());
  std::tr1::unordered_map<string, msgpack::object> map;
  msgpack::object obj3 = msg2.get();

  vector<Node> remoteKnownNodes;

  try {
    obj3.convert(&map);
    map["key"].convert(&key);
    map["knownNodes"].convert(&remoteKnownNodes);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  knownNodes.push_back(*this);
  addNodes(remoteKnownNodes);

  return "OK";
}
