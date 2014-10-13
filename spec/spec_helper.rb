require 'bundler'
Bundler.require(:default, :test)
require_relative '../client/zero10dht.rb'


RSpec.configure do |config|
  config.before(:all) do
    port = 2391
    @pid = Process.spawn("bin/010dhtd #{port}")
    sleep 0.5
    $zero10dht = Zero10DHT.connect port: port
  end
  config.after(:all) do
    Process.kill('HUP', @pid)
  end
end
