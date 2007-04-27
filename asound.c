#include "ruby.h"
#include <alsa/asoundlib.h>

static VALUE m_snd, m_seq;
static VALUE c_seq, c_event, c_client_info, c_port_info;

static VALUE
seq_allocate(VALUE klass)
{
    VALUE result;
    snd_seq_t **seq;
    result = Data_Make_Struct(klass, snd_seq_t *, 0, free, seq);
    snd_seq_open(seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    return result;
}

static VALUE
seq_set_client_name(VALUE self, VALUE name)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_set_client_name(*seq, STR2CSTR(name));
    return Qnil;
}

static VALUE
seq_create_simple_port(VALUE self, VALUE name, VALUE caps, VALUE type)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    return INT2NUM(snd_seq_create_simple_port(*seq, STR2CSTR(name), NUM2INT(caps), NUM2INT(type)));
}

static VALUE
seq_connect_from(VALUE self, VALUE port, VALUE other_client, VALUE other_port)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_connect_from(*seq, NUM2INT(port), NUM2INT(other_client), NUM2INT(other_port));
    return Qnil;
}

static VALUE
seq_connect_to(VALUE self, VALUE port, VALUE other_client, VALUE other_port)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_connect_to(*seq, NUM2INT(port), NUM2INT(other_client), NUM2INT(other_port));
    return Qnil;
}

static VALUE
seq_event_output(VALUE self, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    Data_Get_Struct(event, snd_seq_event_t, ev);
    return INT2NUM(snd_seq_event_output(*seq, ev));
}

static VALUE
seq_event_output_direct(VALUE self, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    Data_Get_Struct(event, snd_seq_event_t, ev);
    return INT2NUM(snd_seq_event_output_direct(*seq, ev));
}

static VALUE
seq_drain_output(VALUE self)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    return INT2NUM(snd_seq_drain_output(*seq));
}

static VALUE
seq_drop_output(VALUE self)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    return INT2NUM(snd_seq_drop_output(*seq));
}

static VALUE
seq_alloc_queue(VALUE self)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    return INT2NUM(snd_seq_alloc_queue(*seq));
}

static VALUE
seq_event_input(VALUE self)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    int result;
    Data_Get_Struct(self, snd_seq_t *, seq);
    result = INT2NUM(snd_seq_event_input(*seq, &ev));
    if (result >= 0) {
	return Data_Wrap_Struct(c_event, 0, 0, ev);
    }
    else {
	return Qnil;
    }
}

static VALUE
seq_event_input_pending(VALUE self, VALUE fetch_sequencer)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    return INT2NUM(snd_seq_event_input_pending(*seq, NUM2INT(fetch_sequencer)));
}

static VALUE
seq_nonblock(VALUE self, VALUE flag)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_nonblock(*seq, RTEST(flag));
    return Qnil;
}

static VALUE
seq_change_queue_ppq(VALUE self, VALUE queue, VALUE ppq)
{
    snd_seq_t **seq;
    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    snd_seq_queue_tempo_set_tempo(tempo, 2000000); // 120 BPM
    snd_seq_queue_tempo_set_ppq(tempo, NUM2INT(ppq));
    snd_seq_set_queue_tempo(*seq, NUM2INT(queue), tempo);
    return Qnil;
}

static VALUE
seq_change_queue_tempo(VALUE self, VALUE queue, VALUE tempo, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    if (NIL_P(event)) {
	ev = NULL;
    }
    else {
	Data_Get_Struct(event, snd_seq_event_t, ev);
    }
    snd_seq_change_queue_tempo(*seq, NUM2INT(queue), NUM2INT(tempo), ev);
    return Qnil;
}

static VALUE
seq_start_queue(VALUE self, VALUE queue, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    if (NIL_P(event)) {
	ev = NULL;
    }
    else {
	Data_Get_Struct(event, snd_seq_event_t, ev);
    }
    snd_seq_start_queue(*seq, NUM2INT(queue), ev);
    return Qnil;
}

static VALUE
seq_stop_queue(VALUE self, VALUE queue, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    if (NIL_P(event)) {
	ev = NULL;
    }
    else {
	Data_Get_Struct(event, snd_seq_event_t, ev);
    }
    snd_seq_stop_queue(*seq, NUM2INT(queue), ev);
    return Qnil;
}

static VALUE
seq_continue_queue(VALUE self, VALUE queue, VALUE event)
{
    snd_seq_t **seq;
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_t *, seq);
    if (NIL_P(event)) {
	ev = NULL;
    }
    else {
	Data_Get_Struct(event, snd_seq_event_t, ev);
    }
    snd_seq_continue_queue(*seq, NUM2INT(queue), ev);
    return Qnil;
}

static VALUE
seq_queue_get_tick_time(VALUE self, VALUE queue)
{
    snd_seq_t **seq;
    snd_seq_queue_status_t *info;

    Data_Get_Struct(self, snd_seq_t *, seq);
    snd_seq_queue_status_alloca(&info);
    snd_seq_get_queue_status(*seq, NUM2INT(queue), info);
    return INT2NUM(snd_seq_queue_status_get_tick_time(info));

}

static VALUE
seq_get_client_info(VALUE self, VALUE client_info_param)
{
    snd_seq_t **seq;
    snd_seq_client_info_t *client_info;
    Data_Get_Struct(self, snd_seq_t *, seq);
    Data_Get_Struct(client_info_param, snd_seq_client_info_t, client_info);
    return INT2NUM(snd_seq_get_client_info(*seq, client_info));
}

static VALUE
seq_query_next_client(VALUE self, VALUE client_info_param)
{
    snd_seq_t **seq;
    snd_seq_client_info_t *client_info;
    Data_Get_Struct(self, snd_seq_t *, seq);
    Data_Get_Struct(client_info_param, snd_seq_client_info_t, client_info);
    return INT2NUM(snd_seq_query_next_client(*seq, client_info));
}

static VALUE
seq_query_next_port(VALUE self, VALUE port_info_param)
{
    snd_seq_t **seq;
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_t *, seq);
    Data_Get_Struct(port_info_param, snd_seq_port_info_t, port_info);
    return INT2NUM(snd_seq_query_next_port(*seq, port_info));
}

static VALUE
ev_allocate(VALUE klass)
{
    VALUE result;
    snd_seq_event_t *ev;
    result = Data_Make_Struct(klass, snd_seq_event_t, 0, free, ev);
    snd_seq_ev_clear(ev);
    return result;
}

static VALUE
ev_set_source(VALUE self, VALUE port)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_source(ev, NUM2INT(port));
    return Qnil;
}

static VALUE
ev_set_subs(VALUE self)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_subs(ev);
    return Qnil;
}

static VALUE
ev_set_direct(VALUE self)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_direct(ev);
    return Qnil;
}

static VALUE
ev_type(VALUE self)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    return INT2NUM(ev->type);
}

static VALUE
ev_set_queue_pos_tick(VALUE self, VALUE queue, VALUE tick)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_queue_pos_tick(ev, NUM2INT(queue), NUM2INT(tick));
    return Qnil;
}

static VALUE
ev_set_queue_tempo(VALUE self, VALUE queue, VALUE tempo)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_queue_tempo(ev, NUM2INT(queue), NUM2INT(tempo));
    return Qnil;
}

static VALUE
ev_set_note(VALUE self, VALUE ch, VALUE note, VALUE velocity, VALUE duration)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_note(ev, NUM2INT(ch), NUM2INT(note), NUM2INT(velocity), NUM2INT(duration));
    return Qnil;
}

static VALUE
ev_set_noteon(VALUE self, VALUE ch, VALUE note, VALUE velocity)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_noteon(ev, NUM2INT(ch), NUM2INT(note), NUM2INT(velocity));
    return Qnil;
}

static VALUE
ev_set_noteoff(VALUE self, VALUE ch, VALUE note, VALUE velocity)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_noteoff(ev, NUM2INT(ch), NUM2INT(note), NUM2INT(velocity));
    return Qnil;
}

static VALUE
ev_set_controller(VALUE self, VALUE ch, VALUE cc, VALUE value)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_controller(ev, NUM2INT(ch), NUM2INT(cc), NUM2INT(value));
    return Qnil;
}

static VALUE
ev_set_pgmchange(VALUE self, VALUE ch, VALUE value)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_pgmchange(ev, NUM2INT(ch), NUM2INT(value));
    return Qnil;
}

static VALUE
ev_set_pitchbend(VALUE self, VALUE ch, VALUE value)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_pitchbend(ev, NUM2INT(ch), NUM2INT(value));
    return Qnil;
}

static VALUE
ev_set_sysex(VALUE self, VALUE data)
{
    snd_seq_event_t *ev;
    char *str;
    long len;
    str = rb_str2cstr(data, &len);
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_sysex(ev, len, str);
    return INT2NUM(len);
}

static VALUE
ev_get_variable(VALUE self)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    return rb_str_new(ev->data.ext.ptr, ev->data.ext.len);
}

static VALUE
ev_schedule_tick(VALUE self, VALUE queue, VALUE relative, VALUE time)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_schedule_tick(ev, NUM2INT(queue), NUM2INT(relative), NUM2INT(time));
    return Qnil;
}

static VALUE
ev_get_type(VALUE self)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    return INT2NUM(ev->type);
}

static VALUE
ev_get_source(VALUE self)
{
    snd_seq_event_t *ev;
    VALUE result;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    result = rb_ary_new();
    rb_ary_push(result, INT2NUM(ev->source.client));
    rb_ary_push(result, INT2NUM(ev->source.port));
    return result;
}

static VALUE
ev_get_dest(VALUE self)
{
    snd_seq_event_t *ev;
    VALUE result;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    result = rb_ary_new();
    rb_ary_push(result, INT2NUM(ev->dest.client));
    rb_ary_push(result, INT2NUM(ev->dest.port));
    return result;
}

static VALUE
ev_set_dest(VALUE self, VALUE client, VALUE port)
{
    snd_seq_event_t *ev;
    Data_Get_Struct(self, snd_seq_event_t, ev);
    snd_seq_ev_set_dest(ev, NUM2INT(client), NUM2INT(port));
    return Qnil;
}

static VALUE
client_info_allocate(VALUE klass)
{
    snd_seq_client_info_t *client_info;
    int malloc_result;
    malloc_result = snd_seq_client_info_malloc(&client_info);
    if (malloc_result) {
	return Qnil;
    }
    else {
	return Data_Wrap_Struct(klass, 0, free, client_info);
    }
}

static VALUE
client_info_get_client(VALUE self)
{
    snd_seq_client_info_t *client_info;
    Data_Get_Struct(self, snd_seq_client_info_t, client_info);
    return INT2NUM(snd_seq_client_info_get_client(client_info));
}

static VALUE
client_info_set_client(VALUE self, VALUE client)
{
    snd_seq_client_info_t *client_info;
    Data_Get_Struct(self, snd_seq_client_info_t, client_info);
    snd_seq_client_info_set_client(client_info, NUM2INT(client));
    return Qnil;
}

static VALUE
client_info_get_name(VALUE self)
{
    snd_seq_client_info_t *client_info;
    Data_Get_Struct(self, snd_seq_client_info_t, client_info);
    return rb_str_new2(snd_seq_client_info_get_name(client_info));
}

static VALUE
port_info_allocate(VALUE klass)
{
    snd_seq_port_info_t *port_info;
    int malloc_result;
    malloc_result = snd_seq_port_info_malloc(&port_info);
    if (malloc_result) {
	return Qnil;
    }
    else {
	return Data_Wrap_Struct(klass, 0, free, port_info);
    }
}

static VALUE
port_info_get_client(VALUE self)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    return INT2NUM(snd_seq_port_info_get_client(port_info));
}

static VALUE
port_info_set_client(VALUE self, VALUE client)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    snd_seq_port_info_set_client(port_info, NUM2INT(client));
    return Qnil;
}

static VALUE
port_info_get_port(VALUE self)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    return INT2NUM(snd_seq_port_info_get_port(port_info));
}

static VALUE
port_info_set_port(VALUE self, VALUE port)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    snd_seq_port_info_set_port(port_info, NUM2INT(port));
    return Qnil;
}

static VALUE
port_info_get_name(VALUE self)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    return rb_str_new2(snd_seq_port_info_get_name(port_info));
}

static VALUE
port_info_get_capability(VALUE self)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    return INT2NUM(snd_seq_port_info_get_capability(port_info));
}

static VALUE
port_info_get_type(VALUE self)
{
    snd_seq_port_info_t *port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    return INT2NUM(snd_seq_port_info_get_type(port_info));
}

static VALUE
port_info_copy_from(VALUE self, VALUE other)
{
    snd_seq_port_info_t *port_info;
    snd_seq_port_info_t *other_port_info;
    Data_Get_Struct(self, snd_seq_port_info_t, port_info);
    Data_Get_Struct(other, snd_seq_port_info_t, other_port_info);
    snd_seq_port_info_copy(port_info, other_port_info);
    return Qnil;
}

void
Init__asound(void)
{
    m_snd = rb_define_module("Snd");
    m_seq = rb_define_module_under(m_snd, "Seq");

#define CONST(name) rb_define_const(m_seq, #name, INT2NUM(SND_SEQ_##name))
    CONST( PORT_CAP_READ );
    CONST( PORT_CAP_WRITE );
    CONST( PORT_CAP_SYNC_READ );
    CONST( PORT_CAP_SYNC_WRITE );
    CONST( PORT_CAP_DUPLEX );
    CONST( PORT_CAP_SUBS_READ );
    CONST( PORT_CAP_SUBS_WRITE );
    CONST( PORT_CAP_NO_EXPORT );
    CONST( PORT_TYPE_MIDI_GENERIC );
    CONST( EVENT_NOTEON );
    CONST( EVENT_NOTEOFF );
    CONST( EVENT_KEYPRESS );
    CONST( EVENT_CONTROLLER );
    CONST( EVENT_PGMCHANGE );
    CONST( EVENT_CHANPRESS );
    CONST( EVENT_PITCHBEND );
    CONST( EVENT_CLOCK );
    CONST( EVENT_SYSEX );
#undef CONST

    c_seq = rb_define_class_under(m_seq, "SequencerClient", rb_cObject);
    rb_define_alloc_func(c_seq, seq_allocate);
    rb_define_method(c_seq, "client_name=", seq_set_client_name, 1);
    rb_define_method(c_seq, "nonblocking=", seq_nonblock, 1);
    rb_define_method(c_seq, "_create_simple_port", seq_create_simple_port, 3);
    rb_define_method(c_seq, "connect_from", seq_connect_from, 3);
    rb_define_method(c_seq, "connect_to", seq_connect_to, 3);
    rb_define_method(c_seq, "event_output", seq_event_output, 1);
    rb_define_method(c_seq, "event_output_direct", seq_event_output_direct, 1);
    rb_define_method(c_seq, "drain_output", seq_drain_output, 0);
    rb_define_method(c_seq, "drop_output", seq_drop_output, 0);
    rb_define_method(c_seq, "event_input", seq_event_input, 0);
    rb_define_method(c_seq, "event_input_pending", seq_event_input_pending, 1);
    rb_define_method(c_seq, "_alloc_queue", seq_alloc_queue, 0);
    rb_define_method(c_seq, "change_queue_ppq", seq_change_queue_ppq, 2);
    rb_define_method(c_seq, "_change_queue_tempo", seq_change_queue_tempo, 3);
    rb_define_method(c_seq, "_start_queue", seq_start_queue, 2);
    rb_define_method(c_seq, "_stop_queue", seq_stop_queue, 2);
    rb_define_method(c_seq, "_continue_queue", seq_continue_queue, 2);
    rb_define_method(c_seq, "queue_get_tick_time", seq_queue_get_tick_time, 1);
    rb_define_method(c_seq, "get_client_info", seq_get_client_info, 1);
    rb_define_method(c_seq, "query_next_client", seq_query_next_client, 1);
    rb_define_method(c_seq, "query_next_port", seq_query_next_port, 1);

    c_event = rb_define_class_under(m_seq, "Event", rb_cObject);
    rb_define_alloc_func(c_event, ev_allocate);
    rb_define_method(c_event, "source=", ev_set_source, 1);
    rb_define_method(c_event, "set_subs", ev_set_subs, 0);
    rb_define_method(c_event, "set_direct", ev_set_direct, 0);
    rb_define_method(c_event, "type", ev_type, 0);
    rb_define_method(c_event, "set_queue_pos_tick", ev_set_queue_pos_tick, 2);
    rb_define_method(c_event, "set_queue_tempo", ev_set_queue_tempo, 2);
    rb_define_method(c_event, "set_note", ev_set_note, 4);
    rb_define_method(c_event, "set_noteon", ev_set_noteon, 3);
    rb_define_method(c_event, "set_noteoff", ev_set_noteoff, 3);
    rb_define_method(c_event, "set_controller", ev_set_controller, 3);
    rb_define_method(c_event, "set_pgmchange", ev_set_pgmchange, 2);
    rb_define_method(c_event, "set_pitchbend", ev_set_pitchbend, 2);
    rb_define_method(c_event, "set_sysex", ev_set_sysex, 1);
    rb_define_method(c_event, "variable", ev_get_variable, 0);
    rb_define_method(c_event, "schedule_tick", ev_schedule_tick, 3);
    rb_define_method(c_event, "type", ev_get_type, 0);
    rb_define_method(c_event, "source", ev_get_source, 0);
    rb_define_method(c_event, "dest", ev_get_dest, 0);
    rb_define_method(c_event, "set_dest", ev_set_dest, 2);

    c_client_info = rb_define_class_under(m_seq, "ClientInfo", rb_cObject);
    rb_define_alloc_func(c_client_info, client_info_allocate);
    rb_define_method(c_client_info, "client", client_info_get_client, 0);
    rb_define_method(c_client_info, "client=", client_info_set_client, 1);
    rb_define_method(c_client_info, "name", client_info_get_name, 0);

    c_port_info = rb_define_class_under(m_seq, "PortInfo", rb_cObject);
    rb_define_alloc_func(c_port_info, port_info_allocate);
    rb_define_method(c_port_info, "client", port_info_get_client, 0);
    rb_define_method(c_port_info, "client=", port_info_set_client, 1);
    rb_define_method(c_port_info, "port", port_info_get_port, 0);
    rb_define_method(c_port_info, "port=", port_info_set_port, 1);
    rb_define_method(c_port_info, "name", port_info_get_name, 0);
    rb_define_method(c_port_info, "capability", port_info_get_capability, 0);
    rb_define_method(c_port_info, "type", port_info_get_type, 0);
    rb_define_method(c_port_info, "copy_from", port_info_copy_from, 1);
}
