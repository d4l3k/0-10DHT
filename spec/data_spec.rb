require_relative 'spec_helper.rb'

RSpec.describe '0/10 DHT' do
  describe 'SET' do
    it 'should be able to set a value' do
      expect($zero10dht.set('test', 'banana')).to eq('OK')
    end
    it 'should err on an invalid key' do
      expect($zero10dht.set('', 'asdf')).to eq('ERR INVALID KEY')
    end
  end
  describe 'GET' do
    it 'should be able to get a value' do
      $zero10dht.set('test2', 'banana')
      expect($zero10dht.get('test2')).to eq('banana')
    end
    it 'should throw an error on a non-existant key' do
      expect($zero10dht.get('does not exist')).to eq('ERR NOT FOUND')
    end
  end
  describe 'NODEADD' do
    it 'should throw an error on a non-existant node' do
      expect($zero10dht.nodeadd('127.0.0.1', 19239)).to eq('ERR BAD CMD')
    end
    it 'should be able to connect to a node' do
      initial_nodes = $zero10dht.nodes
      pid, node = spawnNode
      expect($zero10dht.nodeadd('127.0.0.1', node.port)).to eq('OK')
      new_nodes = $zero10dht.nodes
      expect(node.nodes.length).to eq(new_nodes.length)
      expect($zero10dht.nodes.length).to eq(initial_nodes.length + 1)
      Process.kill('HUP', pid)
    end
  end
end
