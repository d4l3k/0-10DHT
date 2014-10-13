0/10 DHT
=======

0/10 Distributed Hash Table. It'll probably lose your data and ruin your life.

If you're okay with that feel free to try it out. It's designed to be an in memory database similar to Redis. This will really only excel at large quantities (more than can be stored on a single server) of temporary data.

## Commands

Commands sent to 0/10 DHT nodes are formated with [MessagePack](http://msgpack.org/). It's a binary version of JSON. This means it's a little bit smaller. The commands listed below are written in JSON and must be packed before being sent.

There is a reference implementation of the client written in Ruby in the `client/` folder.

### Data Manipulation Commands

The value needs to be a string.

#### SET

```json
{
  cmd: "SET",
  key: "<key>",
  val: <val>
}
```
Returns `OK`.


#### GET
```json
{
  cmd: "GET",
  key: "<key>"
}
```
Returns the corresponding value or `ERR INVALID KEY`.


### Node Manipulation Commands

These commands are mostly used for setting up new nodes or internal communications between the nodes.

#### NODES
```json
{
  cmd: "NODES"
}
```
Returns a msgpack formated array of nodes known to the currently connected node.


#### NODEADD
Add a node to the DHT.
```json
{
  cmd: "NODEADD",
  host: "<ip address>",
  port: <port number>
}
```


#### NODEINTRO
Internal command used by a node to introduce itself to another node.
```json
{
  cmd: "NODEINTRO",
  port: <port number>,
  key: <uint64 node address>,
  knownNodes: [{
      host: "<ip address>",
      port: <port number>
      key: <uint64 node address>
    },
    ...
  ]
```
Returns
```json
{
  key: <uint64 node address>,
  knownNodes: [<see above>]
}
```


#### NODEKEYS
```json
{
  cmd: "NODEKEYS"
}
```
Returns a string array of keys stored on the current node.


#### NODEDUMPMAP
```json
{
  cmd: "NODEDUMPMAP"
}
```
Dumps a map of the entire database.

## Development

### Dependencies

0/10 DHT has several dependencies. These must be available on the compiling system.

* [sparsehash](https://code.google.com/p/sparsehash/)
* [cityhash](https://code.google.com/p/cityhash/)
* [msgpack-c](https://github.com/msgpack/msgpack-c)

On Arch Linux these are available from the AUR: `sparsehash cityhash msgpack`.

### Running
```bash
make -j4 && bin/010dhtd 8001
```
This compiles the DHT and launches a node on port 8001.

### Tests
The tests are written in Ruby & RSpec.

```bash
# install Bundler if you don't have it
gem install bundler

# install the Ruby dependencies
bundle

# run the tests
rspec
```

## License

Copyright (C) 2014 Tristan Rice

0/10 DHT is licensed under the MIT License.
