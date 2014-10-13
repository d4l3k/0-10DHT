require_relative 'spec_helper.rb'

RSpec.describe '0/10 DHT' do
  describe 'Data Functions' do
    it 'should be able to set a value' do
      expect($zero10dht.set('test', 'banana')).to eq('OK')
    end
    it 'should be able to set and get a value' do
      $zero10dht.set('test2', 'banana')
      expect($zero10dht.get('test2')).to eq('banana')
    end
  end
end
