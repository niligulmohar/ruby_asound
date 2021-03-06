require 'test/unit'

require 'asound'

class AlsaTestCase < Test::Unit::TestCase
  def setup
    @seq = Snd::Seq::open
    @seq.client_name = 'Asound unit test'
  end
  def test_functionality_and_timing
    p = @seq.create_simple_port('Test port',
                                Snd::Seq::PORT_CAP_READ | Snd::Seq::PORT_CAP_SUBS_READ,
                                Snd::Seq::PORT_TYPE_MIDI_GENERIC)
    p.connect_to([129, 0])

    q = @seq.alloc_queue
    q.ppq = 960
    q.tempo = 120
    q.start
    @seq.drain_output

    ev = Snd::Seq::Event.new
    ev.source = p
    ev.set_subs
    ev.set_direct
    ev.set_noteon(1, 64, 100)
    assert_equal(6, ev.type)
    @seq.event_output(ev)

    ev = Snd::Seq::Event.new
    ev.source = p
    ev.set_subs
    ev.schedule_tick(q, 0, 96*4)
    ev.set_noteoff(1, 64, 0)
    @seq.event_output(ev)

    @seq.drain_output

    start = q.tick_time
    assert_equal(3, sleep(3))
    stop = q.tick_time

    q.stop
    q.continue
    @seq.drain_output
    continue = q.tick_time

    q.stop
    q.start
    @seq.drain_output
    assert_equal(1, sleep(1))
    restart = q.tick_time

    assert_in_delta(0, start, 2)
    assert_in_delta(5760, stop, 2)
    assert_in_delta(5760, continue, 2)
    assert_in_delta(1920, restart, 2)
  end
end
