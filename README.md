0/10 DHT
=======

0/10 Distributed Hash Table. It'll probably lose your data and ruin your life.


## Commands

Commands sent to 0/10 DHT nodes are formated with [MessagePack](http://msgpack.org/). It's a binary version of JSON. This means it's a little bit smaller. The commands listed below are written in JSON and must be packed before being sent.

### Data Manipulation Commands

#### SET

```
{
  cmd: "SET",
  key: "<key>",
  val: "<val>"
}
```
Returns `OK`.

#### GET
```
{
  cmd: "GET",
  key: "<key>"
}
```
Returns the corresponding value or `ERR INVALID KEY`.

### Node Manipulation Commands

These commands are mostly used for setting up new nodes or internal communications between the nodes.

#### NODES
```
{
  cmd: "NODES"
}
```
Returns a msgpack formated array of nodes known to the currently connected node.

#### NODEADD
Add a node to the DHT.
```
{
  cmd: "NODEADD",
  host: "<ip address>",
  port: <port number>
}
```

#### NODEINTRO
Internal command used by a node to introduce itself to another node.
```
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
```
{
  key: <uint64 node address>,
  knownNodes: [<see above>]
}
```

#### NODEKEYS
```
{
  cmd: "NODEKEYS"
}
```
Returns a string array of keys stored on the current node.

#### NODEDUMPMAP
```
{
  cmd: "NODEDUMPMAP"
}
```
Dumps a map of the entire database.
