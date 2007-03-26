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
      Port.new(self, _create_simple_port(name, caps, type))
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
          yield port_info
        end
      end
    end
  end
  class Port
    def initialize(seq, port_info_or_n)
      @seq = seq
      if port_info_or_n.kind_of?(PortInfo)
        @port_info = port_info_or_n
        @n = @port_info_or_n.port
      else
        @n = port_info_or_n
      end
    end
    def to_int() @n; end
    def connect_to(arg)
      if arg.kind_of?(PortInfo)
        @seq.connect_to(@n, arg.client, arg.port)
      else
        @seq.connect_to(@n, *arg)
      end
    end
    def connect_from(arg)
      if arg.kind_of?(PortInfo)
        @seq.connect_from(@n, arg.client, arg.port)
      else
        @seq.connect_from(@n, *arg)
      end
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
    def sysex?
      type == Snd::Seq::EVENT_SYSEX
    end
    def clock?
      type == Snd::Seq::EVENT_CLOCK
    end
    def identity_response?
      sysex? and variable_data =~ /\x7e.\x06\x02.........\xf7/
    end
    def sysex_channel
      fail unless sysex?
      return variable_data[2]
    end
    def destination=(arg)
      set_dest(*arg)
    end
  end
  class PortInfo
    alias_method :capabilities, :capability
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
end
