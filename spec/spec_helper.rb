require 'bundler'
Bundler.require(:default, :test)
require_relative '../client/zero10dht.rb'

def spawnNode
    system("make -j4")
    port = 2000 + rand(10000)
    pid = Process.spawn("bin/010dhtd #{port}")
    sleep 0.1
    return [pid, Zero10DHT.connect(port: port)]
end

RSpec.configure do |config|
  config.before(:all) do
    @pid, $zero10dht = spawnNode
  end
  config.after(:all) do
    Process.kill('HUP', @pid)
  end
end
