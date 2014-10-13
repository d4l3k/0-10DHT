require 'msgpack'

class Zero10DHT
  attr_reader :host, :port
  def initialize host, port
    @host = host
    @port = port
    @sock = TCPSocket.new(host, port)
  end
  def nodes
    resp = send({cmd: 'NODES'})
    begin
      MessagePack.unpack(resp)
    rescue
      MessagePack.unpack(resp[0..-2])
    end
  end
  def set key, value
    send({
      cmd: 'SET',
      key: key,
      val: value.to_s
    })
  end
  def get key
    send({
      cmd: 'GET',
      key: key
    })
  end
  def nodeadd host, port
    send({
      cmd: 'NODEADD',
      host: host,
      port: port.to_i
    })
  end
  def nodekeys
    MessagePack.unpack(send({cmd: 'NODEKEYS'}))
  end
  def query cmd, key, val
    send({
      cmd: cmd,
      key: key,
      val: val
    })
  end
  def send obj
    if obj.has_key? :key
      obj[:key] = obj[:key].to_s
    end
    @sock.write(MessagePack.pack(obj))
    @sock.recv(2048)
  end
  def self.connect host: 'localhost', port: ARGV[0] || 8001
    Zero10DHT.new(host, port)
  end
end
