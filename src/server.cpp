/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <msgpack.hpp>
#include <netinet/in.h>
#include <sparsehash/dense_hash_map>
#include <ext/hash_map>
#include <boost/thread.hpp>
#include <signal.h>

#include <tr1/functional>
#include <tr1/unordered_map>
#include "msgpack/type/tr1/unordered_map.hpp"
#include <iostream>
#include <fstream>

//#define DEBUG

using namespace std;

using google::dense_hash_map;      // namespace where class lives by default
using std::cout;
using std::endl;
using __gnu_cxx::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS

void dostuff(int); /* function prototype */
std::string processCmd(const char*);
void error(const char *msg)
{
  cout << "ERROR: " << msg << endl;
  exit(1);
}


struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};

dense_hash_map<const char*, std::string, hash<const char*>, eqstr> datastore;
boost::mutex datastore_mtx;           // mutex for critical section
struct StringToIntSerializer {
  bool operator()(FILE* fp, const std::pair<const char*, std::string>& value) const {

    msgpack::sbuffer sbuf;

    sbuf.write("\n", 1);

    msgpack::packer<msgpack::sbuffer> pk2(&sbuf);
    pk2.pack_map(3);
    pk2.pack(std::string("cmd"));
    pk2.pack(std::string("SET"));
    pk2.pack(std::string("key"));
    pk2.pack(std::string(value.first));
    pk2.pack(std::string("val"));
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
  bool operator()(FILE* fp, std::pair<const char*, std::string>* value) const {
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

    std::string line;
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
  FILE* fp = fopen("weedht.db", "w");
  datastore.serialize(StringToIntSerializer(), fp);
  datastore_mtx.unlock();
  fclose(fp);
  cout << "Done!" << endl;
  return 0;
}

void sighandler(int sig)
{
    cout<< "Signal " << sig << " caught..." << endl;
    save();
    exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno, pid;
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

  datastore.set_empty_key("");

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
      std::string resp = processCmd(buffer);
      n = write(sock, resp.c_str(), resp.size());
      if (n < 0) error("ERROR writing to socket");
#ifdef DEBUG
      cout << "RESP " << resp << endl;
#endif
    }
  }
}
std::string processCmd(const char* buffer) {
  std::string resp;
  // Deserialize the serialized data.
  msgpack::unpacked msg;
  msgpack::unpack(&msg, buffer, 256);
  msgpack::object obj = msg.get();

  std::tr1::unordered_map<std::string, std::string> map;
  try {
    obj.convert(&map);
  } catch (msgpack::type_error) {
    return "ERR BAD CMD";
  }

  std::string cmd = map.find("cmd")->second;
  std::string key = map.find("key")->second;
#ifdef DEBUG
  cout << obj << endl;
#endif
  if (cmd == "SET") {
    std::string val = map.find("val")->second;
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
  } else {
    resp = "ERR UNKNOWN CMD";
  }
  return resp;
}