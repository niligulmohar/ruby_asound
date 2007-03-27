require 'forwardable'
require '_asound'

module Snd::Seq
  class << self
    def open
      Snd::Seq::Seq.new
    end
  end
  class Seq
    attr_reader :client_id
    def initialize
      @client_id = client_info.client
    end
    def create_simple_port(name, caps, type)
      port_info = PortInfo.new
      port_info.port = _create_simple_port(name, caps, type)
      Port.new(self, port_info)
    end
    def alloc_queue
      Queue.new(self, _alloc_queue)
    end
    def change_queue_tempo(q, tempo, ev = nil)
      _change_queue_tempo(q, tempo, ev)
    end
    def start_queue(q, ev = nil)
      _start_queue(q, ev)
    end
    def stop_queue(q, ev = nil)
      _stop_queue(q, ev)
    end
    def continue_queue(q, ev = nil)
      _continue_queue(q, ev)
    end
    def client_info
      client_info = ClientInfo.new
      get_client_info(client_info)
      return client_info
    end
    def each_port
      client_info = ClientInfo.new
      client_info.client = -1

      while (0 == query_next_client(client_info))
        next if client_info.client == @client_id
        port_info = PortInfo.new
        port_info.client = client_info.client
        port_info.port = -1
        while (0 == query_next_port(port_info))
          yield Port.new(self, port_info.clone)
        end
      end
    end
    def input_pending?
      event_input_pending(0)
    end
  end
  class Queue
    def initialize(seq, n)
      @seq = seq
      @n = n
    end
    def to_int() @n; end
    def ppq=(ppq)
      @seq.change_queue_ppq(@n, ppq)
    end
    def change_tempo(bpm, ev = nil)
      @seq.change_queue_tempo(@n, 60000000 / bpm, ev)
    end
    def tempo=(bpm)
      change_tempo(bpm)
    end
    def start(ev = nil)
      @seq.start_queue(@n, ev)
    end
    def stop(ev = nil)
      @seq.stop_queue(@n, ev)
    end
    def continue(ev = nil)
      @seq.continue_queue(@n, ev)
    end
    def tick_time
      @seq.queue_get_tick_time(@n)
    end
  end
  class Event
    alias_method :variable_data, :variable
    alias_method :to_port_subscribers!, :set_subs
    alias_method :direct!, :set_direct

    def self.def_type_checks(*name_syms)
      name_syms.each do |name_sym|
        name = name_sym.to_s
        class_eval("def #{name}?() type == Snd::Seq::EVENT_#{name.upcase} end")
      end
    end
    def_type_checks :sysex, :clock, :noteon, :noteoff, :keypress, :controller
    def_type_checks :pgmchange, :chanpress, :pitchbend

    def identity_response?
      sysex? and variable_data =~ /^\xf0\x7e.\x06\x02.........\xf7$/
    end
    def sysex_channel
      fail unless sysex?
      return variable_data[2]
    end
    def destination=(arg)
      set_dest(*arg)
    end
    def source_info
      port_info = PortInfo.new
      port_info.client = source[0]
      port_info.port = source[1]
      return port_info
    end
    def source_ids
      "[#{source[0]}:#{source[1]}]"
    end
    def to_s
      if sysex?
        'System exclusive'
      elsif noteon?
        'Note on'
      elsif noteoff?
        'Note off'
      elsif keypress?
        'Key aftertouch'
      elsif controller?
        'Continuous controller'
      elsif pgmchange?
        'Program change'
      elsif chanpress?
        'Channel aftertouch'
      elsif pitchbend?
        'Pitch bend'
      end
    end
  end
  class PortInfo
    alias_method :capabilities, :capability
    alias_method :to_int, :port
    def clone
      copy = super
      copy.copy_from(self)
      return copy
    end
    def midi?
      type & PORT_TYPE_MIDI_GENERIC != 0
    end
    def readable?
      capabilities & PORT_CAP_READ != 0
    end
    def writable?
      capabilities & PORT_CAP_WRITE != 0
    end
    def read_subscribable?
      capabilities & PORT_CAP_SUBS_READ != 0
    end
    def write_subscribable?
      capabilities & PORT_CAP_SUBS_WRITE != 0
    end
  end
  class Port
    attr_reader :seq, :port_info
    def initialize(seq, port_info)
      @seq = seq
      @port_info = port_info
    end
    def method_missing(sym, *args)
      @port_info.send(sym, *args)
    end
    def to_int
      @port_info.port
    end
    def connect_to(arg)
      if arg.kind_of?(PortInfo)
        @seq.connect_to(@port_info.port, arg.client, arg.port)
      elsif arg.kind_of?(Port)
        connect_to(arg.port_info)
      else
        @seq.connect_to(@port_info.port, *arg)
      end
    end
    def connect_from(arg)
      if arg.kind_of?(PortInfo)
        @seq.connect_from(@port_info.port, arg.client, arg.port)
      elsif arg.kind_of?(Port)
        connect_from(arg.port_info)
      else
        @seq.connect_from(@port_info.port, *arg)
      end
    end
    def event_output!(event_param = nil)
      event = event_param || Event.new
      yield event if block_given?
      event.to_port_subscribers!
      @seq.event_output(event)
      @seq.drain_output
    end
    def ids
      "[#{@port_info.client}:#{@port_info.port}]"
    end
  end
  class DestinationPort
    def initialize(port, source_port)
      @port = port
      @source_port = source_port
    end
    def method_missing(sym, *args)
      @port.send(sym, *args)
    end
    def event_output!(event_param = nil)
      event = event_param || Event.new
      yield event if block_given?
      event.destination = [@port.client, @port.port]
      event.source = [@source_port.client, @source_port.port]
      @seq.event_output(event)
      @seq.drain_output
    end
    def ids
      "[#{@port.client}:#{@port.port}]"
    end
  end
end
