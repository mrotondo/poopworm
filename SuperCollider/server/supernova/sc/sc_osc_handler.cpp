//  osc handler for supercollider-style communication, implementation
//  Copyright (C) 2009, 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#include <iostream>

#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPrintReceivedElements.h"

#include "sc_msg_iter.h"
#include "sc_osc_handler.hpp"
#include "../server/server.hpp"
#include "utilities/sized_array.hpp"

namespace nova
{

using namespace std;

namespace
{

int32_t last_generated = 0;

server_node * find_node(int32_t target_id)
{
    if (target_id == -1)
        target_id = last_generated;

    server_node * node = instance->find_node(target_id);

    if (node == NULL)
        cerr << "node not found" << endl;
    return node;
}

abstract_group * find_group(int32_t target_id)
{
    if (target_id == -1)
        target_id = last_generated;

    abstract_group * node = instance->find_group(target_id);

    if (node == NULL)
        cerr << "node not found or not a group" << endl;
    return node;
}

bool check_node_id(int node_id)
{
    if (!instance->node_id_available(node_id)) {
        cerr << "node id " << node_id << " already in use" << endl;
        return false;
    }
    return true;
}

void fill_notification(const server_node * node, osc::OutboundPacketStream & p)
{
    p << node->id();

    /* parent */
    const abstract_group * parent_node = node->get_parent();
    assert(parent_node);
    p << parent_node->id();

    /* previous/next */
    if (parent_node->is_parallel())
        p << -2 << -2; /* we are in a parallel group, so we have no notion of previous/next */
    else
    {
        const server_node * prev_node = node->previous_node();
        if (prev_node)
            p << prev_node->id();
        else
            p << -1;

        const server_node * next_node = node->next_node();
        if (next_node)
            p << next_node->id();
        else
            p << -1;
    }

    /* is_synth, head, tail */
    if (node->is_synth())
        p << 0;
    else {
        const abstract_group * node_group = static_cast<const abstract_group*>(node);
        p << 1;

        if (node_group->is_parallel())
            p << -2 << -2;
        else
        {
            const group * node_real_group = static_cast<const group*>(node_group);
            if (node_real_group->empty())
                p << -1 << -1;
            else
                p << node_real_group->head_node()->id()
                  << node_real_group->tail_node()->id();
        }
    }

    p << osc::EndMessage;
}

spin_lock system_callback_allocator_lock;

struct movable_string
{
    /** allocate new string, only allowed to be called from the rt context */
    explicit movable_string(const char * str)
    {
        size_t length = strlen(str) + 1; /* terminating \0 */
        char * data = (char*)system_callback::allocate(length);
        strcpy(data, str);
        data_ = data;
    }

    /** copy constructor has move semantics!!! */
    movable_string(movable_string const & rhs)
    {
        data_ = rhs.data_;
        const_cast<movable_string&>(rhs).data_ = NULL;
    }

    ~movable_string(void)
    {
        if (data_)
            system_callback::deallocate((char*)data_);
    }

    const char * c_str(void) const
    {
        return data_;
    }

private:
    const char * data_;
};

template <typename T>
struct movable_array
{
    /** allocate new array, only allowed to be called from the rt context */
    movable_array(size_t length, const T * data, bool locked = false):
        length_(length)
    {
        data_ = (T*)system_callback::allocate(length * sizeof(T));
        for (size_t i = 0; i != length; ++i)
            data_[i] = data[i];
    }

    /** copy constructor has move semantics!!! */
    movable_array(movable_array const & rhs)
    {
        length_ = rhs.length_;
        data_ = rhs.data_;
        const_cast<movable_array&>(rhs).data_ = NULL;
    }

    ~movable_array(void)
    {
        if (data_)
            system_callback::deallocate((char*)data_);
    }

    const T * data(void) const
    {
        return data_;
    }

    const T & operator[](size_t index) const
    {
        return data_[index];
    }

    size_t size(void) const
    {
        return length_;
    }

private:
    size_t length_;
    T * data_;
};

void send_done_message(nova_endpoint const & endpoint)
{
    char buffer[128];
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage("/done")
      << osc::EndMessage;

    instance->send(p.Data(), p.Size(), endpoint);
}

void send_done_message(nova_endpoint const & endpoint, const char * cmd)
{
    char buffer[128];
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage("/done")
      << cmd
      << osc::EndMessage;

    instance->send(p.Data(), p.Size(), endpoint);
}

void send_done_message(nova_endpoint const & endpoint, const char * cmd, osc::int32 index)
{
    char buffer[128];
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage("/done")
      << cmd
      << index
      << osc::EndMessage;

    instance->send(p.Data(), p.Size(), endpoint);
}

template <typename Functor>
struct fn_system_callback:
    public system_callback
{
    fn_system_callback (Functor const & fn):
        fn_(fn)
    {}

    void run(void)
    {
        fn_();
    }

    Functor fn_;
};

template <typename Functor>
struct fn_sync_callback:
    public audio_sync_callback
{
    fn_sync_callback (Functor const & fn):
        fn_(fn)
    {}

    void run(void)
    {
        fn_();
    }

    Functor fn_;
};

/** helper class for dispatching real-time and non real-time osc command callbacks
 *
 *  uses template specialization to avoid unnecessary callback rescheduling
 */
template <bool realtime>
struct cmd_dispatcher
{
    template <typename Functor>
    static void fire_system_callback(Functor const & f)
    {
        instance->add_system_callback(new fn_system_callback<Functor>(f));
    }

    template <typename Functor>
    static void fire_io_callback(Functor const & f)
    {
        instance->add_io_callback(new fn_system_callback<Functor>(f));
    }

    template <typename Functor>
    static void fire_rt_callback(Functor const & f)
    {
        instance->add_sync_callback(new fn_sync_callback<Functor>(f));
    }

    static void fire_done_message(nova_endpoint const & endpoint)
    {
        fire_io_callback(boost::bind(send_done_message, endpoint));
    }

    static void fire_done_message(nova_endpoint const & endpoint, const char * cmd)
    {
        fire_io_callback(boost::bind(send_done_message, endpoint, cmd));
    }

    static void fire_done_message(nova_endpoint const & endpoint, const char * cmd, osc::int32 index)
    {
        fire_io_callback(boost::bind(send_done_message, endpoint, cmd, index));
    }
};

template <>
struct cmd_dispatcher<false>
{
    template <typename Functor>
    static void fire_system_callback(Functor f)
    {
        f();
    }

    template <typename Functor>
    static void fire_rt_callback(Functor f)
    {
        f();
    }

    template <typename Functor>
    static void fire_io_callback(Functor f)
    {
        f();
    }


    static void fire_done_message(nova_endpoint const & endpoint)
    {
        send_done_message (endpoint);
    }

    static void fire_done_message(nova_endpoint const & endpoint, const char * cmd)
    {
        send_done_message (endpoint, cmd);
    }

    static void fire_done_message(nova_endpoint const & endpoint, const char * cmd, osc::int32 index)
    {
        send_done_message (endpoint, cmd, index);
    }
};

} /* namespace */

namespace detail
{

void fire_notification(movable_array<char> & msg)
{
    instance->send_notification(msg.data(), msg.size());
}

void sc_notify_observers::notify(const char * address_pattern, const server_node * node)
{
    char buffer[128]; // 128 byte should be enough
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage(address_pattern);
    fill_notification(node, p);

    movable_array<char> message(p.Size(), p.Data());
    cmd_dispatcher<true>::fire_io_callback(boost::bind(fire_notification, message));
}

void fire_trigger(int32_t node_id, int32_t trigger_id, float value)
{
    char buffer[128]; // 128 byte should be enough
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage("/tr") << osc::int32(node_id) << osc::int32(trigger_id) << value
      << osc::EndMessage;

    instance->send_notification(p.Data(), p.Size());
}

void sc_notify_observers::send_trigger(int32_t node_id, int32_t trigger_id, float value)
{
    cmd_dispatcher<true>::fire_io_callback(boost::bind(fire_trigger, node_id, trigger_id, value));
}

void free_mem_callback(movable_string & cmd,
                     movable_array<float> & values)
{}

void fire_node_reply(int32_t node_id, int reply_id, movable_string & cmd,
                     movable_array<float> & values)
{
    size_t buffer_size = 1024 + strlen(cmd.c_str()) + values.size()*sizeof(float);

    char * buffer = (buffer_size < 2048) ? (char*)alloca(buffer_size)
                                         : (char*)malloc(buffer_size);

    try {
        osc::OutboundPacketStream p(buffer, buffer_size);
        p << osc::BeginMessage(cmd.c_str()) << osc::int32(node_id) << osc::int32(reply_id);

        for (int i = 0; i != values.size(); ++i)
            p << values[i];

        p << osc::EndMessage;

        instance->send_notification(p.Data(), p.Size());

        cmd_dispatcher<true>::fire_rt_callback(boost::bind(free_mem_callback, cmd, values));
    } catch (...) {
    }

    if (buffer_size >= 2048)
        free(buffer);
}

void sc_notify_observers::send_node_reply(int32_t node_id, int reply_id, const char* command_name,
                                          int argument_count, const float* values)
{
    spin_lock::scoped_lock lock(system_callback_allocator_lock); // called from rt helper threads, so we need to lock the memory pool
    movable_string cmd(command_name);
    movable_array<float> value_array(argument_count, values);

    cmd_dispatcher<true>::fire_io_callback(boost::bind(fire_node_reply, node_id, reply_id, cmd, value_array));
}

void sc_notify_observers::send_notification(const char * data, size_t length)
{
    for (size_t i = 0; i != observers.size(); ++i)
        send_notification(data, length, observers[i]);
}

void sc_notify_observers::send_notification(const char * data, size_t length, nova_endpoint const & endpoint)
{
    nova_protocol const & prot = endpoint.protocol();
    if (prot.family() == AF_INET && prot.type() == SOCK_DGRAM)
    {
        udp::endpoint ep(endpoint.address(), endpoint.port());
        send_udp(data, length, ep);
    }
    else if (prot.family() == AF_INET && prot.type() == SOCK_STREAM)
    {
        tcp::endpoint ep(endpoint.address(), endpoint.port());
        send_tcp(data, length, ep);
    }
}



void sc_scheduled_bundles::bundle_node::run(void)
{
    typedef osc::ReceivedBundleElement bundle_element;
    typedef osc::ReceivedBundle received_bundle;
    typedef osc::ReceivedMessage received_message;

    bundle_element element(data_);

    if (element.IsBundle()) {
        received_bundle bundle(element);
        instance->handle_bundle<true>(bundle, endpoint_);
    } else {
        received_message message(element);
        instance->handle_message<true>(message, element.Size(), endpoint_);
    }
}

void sc_scheduled_bundles::insert_bundle(time_tag const & timeout, const char * data, size_t length,
                                         nova_endpoint const & endpoint)
{
    /* allocate chunk from realtime pool */
    void * chunk = rt_pool.malloc(sizeof(bundle_node) + length+4);
    bundle_node * node = (bundle_node*)chunk;
    char * cpy = (char*)chunk + sizeof(bundle_node);

    memcpy(cpy, data - 4, length+4);

    new(node) bundle_node(timeout, cpy, endpoint);

    bundle_q.insert(*node);
}

void sc_scheduled_bundles::execute_bundles(time_tag const & now)
{
    while(!bundle_q.empty())
    {
        bundle_node & front = *bundle_q.top();

        if (front.timeout_ <= now) {
            front.run();
            bundle_q.erase_and_dispose(bundle_q.top(), &dispose_bundle);
        }
        else
            return;
    }
}


void sc_osc_handler::open_tcp_acceptor(tcp const & protocol, unsigned int port)
{
    tcp_acceptor_.open(protocol);
    tcp_acceptor_.bind(tcp::endpoint(protocol, port));
    tcp_acceptor_.listen();
}

void sc_osc_handler::open_udp_socket(udp const & protocol, unsigned int port)
{
    sc_notify_observers::udp_socket.open(protocol);
    sc_notify_observers::udp_socket.bind(udp::endpoint(protocol, port));
}

bool sc_osc_handler::open_socket(int family, int type, int protocol, unsigned int port)
{
    if (protocol == IPPROTO_TCP)
    {
        if ( type != SOCK_STREAM )
            return false;

        if (family == AF_INET)
            open_tcp_acceptor(tcp::v4(), port);
        else if (family == AF_INET6)
            open_tcp_acceptor(tcp::v6(), port);
        else
            return false;
        return true;
    }
    else if (protocol == IPPROTO_UDP)
    {
        if ( type != SOCK_DGRAM )
            return false;

        if (family == AF_INET)
            open_udp_socket(udp::v4(), port);
        else if (family == AF_INET6)
            open_udp_socket(udp::v6(), port);
        else
            return false;
        start_receive_udp();
        return true;
    }
    return false;
}


void sc_osc_handler::handle_packet_async(const char * data, size_t length,
                                         nova_endpoint const & endpoint)
{
    received_packet * p = received_packet::alloc_packet(data, length, endpoint);

    if (dump_osc_packets == 1) {
        osc_received_packet packet (data, length);
        cout << "received osc packet " << packet << endl;
    }

    instance->add_sync_callback(p);
}

time_tag sc_osc_handler::handle_bundle_nrt(const char * data, size_t length)
{
    osc_received_packet packet(data, length);
    if (!packet.IsBundle())
        throw std::runtime_error("packet needs to be an osc bundle");

    received_bundle bundle(packet);
    handle_bundle<false> (bundle, nova_endpoint());
    return bundle.TimeTag();
}


sc_osc_handler::received_packet *
sc_osc_handler::received_packet::alloc_packet(const char * data, size_t length,
                                              nova_endpoint const & remote_endpoint)
{
    /* received_packet struct and data array are located in one memory chunk */
    void * chunk = received_packet::allocate(sizeof(received_packet) + length);
    received_packet * p = (received_packet*)chunk;
    char * cpy = (char*)(chunk) + sizeof(received_packet);
    memcpy(cpy, data, length);

    new(p) received_packet(cpy, length, remote_endpoint);
    return p;
}

void sc_osc_handler::received_packet::run(void)
{
    instance->handle_packet(data, length, endpoint_);
}

void sc_osc_handler::handle_packet(const char * data, std::size_t length, nova_endpoint const & endpoint)
{
    osc_received_packet packet(data, length);
    if (packet.IsBundle())
    {
        received_bundle bundle(packet);
        handle_bundle<true> (bundle, endpoint);
    }
    else
    {
        received_message message(packet);
        handle_message<true> (message, packet.Size(), endpoint);
    }
}

template <bool realtime>
void sc_osc_handler::handle_bundle(received_bundle const & bundle, nova_endpoint const & endpoint)
{
    time_tag bundle_time = bundle.TimeTag();

    typedef osc::ReceivedBundleElementIterator bundle_iterator;
    typedef osc::ReceivedBundleElement bundle_element;

    if (bundle_time <= now) {
        for (bundle_iterator it = bundle.ElementsBegin(); it != bundle.ElementsEnd(); ++it) {
            bundle_element const & element = *it;

            if (element.IsBundle()) {
                received_bundle inner_bundle(element);
                handle_bundle<realtime>(inner_bundle, endpoint);
            } else {
                received_message message(element);
                handle_message<realtime>(message, element.Size(), endpoint);
            }
        }
    } else {
        for (bundle_iterator it = bundle.ElementsBegin(); it != bundle.ElementsEnd(); ++it) {
            bundle_element const & element = *it;
            scheduled_bundles.insert_bundle(bundle_time, element.Contents(), element.Size(), endpoint);
        }
    }
}

template <bool realtime>
void sc_osc_handler::handle_message(received_message const & message, size_t msg_size,
                                    nova_endpoint const & endpoint)
{
    try {
        if (message.AddressPatternIsUInt32())
            handle_message_int_address<realtime>(message, msg_size, endpoint);
        else
            handle_message_sym_address<realtime>(message, msg_size, endpoint);
    }
    catch (std::exception const & e)
    {
        cerr << e.what() << endl;
    }
}

namespace {

typedef sc_osc_handler::received_message received_message;

enum {
    cmd_none = 0,

    cmd_notify = 1,
    cmd_status = 2,
    cmd_quit = 3,
    cmd_cmd = 4,

    cmd_d_recv = 5,
    cmd_d_load = 6,
    cmd_d_loadDir = 7,
    cmd_d_freeAll = 8,

    cmd_s_new = 9,
    cmd_n_trace = 10,
    cmd_n_free = 11,
    cmd_n_run = 12,
    cmd_n_cmd = 13,
    cmd_n_map = 14,
    cmd_n_set = 15,
    cmd_n_setn = 16,
    cmd_n_fill = 17,
    cmd_n_before = 18,
    cmd_n_after = 19,

    cmd_u_cmd = 20,

    cmd_g_new = 21,
    cmd_g_head = 22,
    cmd_g_tail = 23,
    cmd_g_freeAll = 24,
    cmd_c_set = 25,
    cmd_c_setn = 26,
    cmd_c_fill = 27,

    cmd_b_alloc = 28,
    cmd_b_allocRead = 29,
    cmd_b_read = 30,
    cmd_b_write = 31,
    cmd_b_free = 32,
    cmd_b_close = 33,
    cmd_b_zero = 34,
    cmd_b_set = 35,
    cmd_b_setn = 36,
    cmd_b_fill = 37,
    cmd_b_gen = 38,
    cmd_dumpOSC = 39,

    cmd_c_get = 40,
    cmd_c_getn = 41,
    cmd_b_get = 42,
    cmd_b_getn = 43,
    cmd_s_get = 44,
    cmd_s_getn = 45,
    cmd_n_query = 46,
    cmd_b_query = 47,

    cmd_n_mapn = 48,
    cmd_s_noid = 49,

    cmd_g_deepFree = 50,
    cmd_clearSched = 51,

    cmd_sync = 52,
    cmd_d_free = 53,

    cmd_b_allocReadChannel = 54,
    cmd_b_readChannel = 55,
    cmd_g_dumpTree = 56,
    cmd_g_queryTree = 57,

    cmd_error = 58,

    cmd_s_newargs = 59,

    cmd_n_mapa = 60,
    cmd_n_mapan = 61,
    cmd_n_order = 62,

    cmd_p_new = 63,

    NUMBER_OF_COMMANDS = 64
};


void send_udp_message(movable_array<char> data, nova_endpoint const & endpoint)
{
    instance->send(data.data(), data.size(), endpoint);
}


int first_arg_as_int(received_message const & message)
{
    osc::ReceivedMessageArgumentStream args = message.ArgumentStream();
    osc::int32 val;

    args >> val;

    return val;
}

void quit_perform(nova_endpoint const & endpoint)
{
    send_done_message(endpoint, "/quit");
    instance->terminate();
}

template <bool realtime>
void handle_quit(nova_endpoint const & endpoint)
{
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(quit_perform, endpoint));
}

void notify_perform(bool enable, nova_endpoint const & endpoint)
{
    if (enable)
        instance->add_observer(endpoint);
    else
        instance->remove_observer(endpoint);
    send_done_message(endpoint, "/notify");
}

template <bool realtime>
void handle_notify(received_message const & message, nova_endpoint const & endpoint)
{
    int enable = first_arg_as_int(message);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(notify_perform, bool(enable), endpoint));
}

void status_perform(nova_endpoint const & endpoint)
{
    char buffer[1024];
    typedef osc::int32 i32;
    osc::OutboundPacketStream p(buffer, 1024);
    p << osc::BeginMessage("/status.reply")
      << (i32)1                                    /* unused */
      << (i32)sc_factory->ugen_count()   /* ugens */
      << (i32)instance->synth_count()     /* synths */
      << (i32)instance->group_count()     /* groups */
      << (i32)instance->prototype_count() /* synthdefs */
      << instance->cpu_load()                 /* average cpu % */
      << instance->cpu_load()                 /* peak cpu % */
      << instance->get_samplerate()           /* nominal samplerate */
      << instance->get_samplerate()           /* actual samplerate */
      << osc::EndMessage;

    instance->send(p.Data(), p.Size(), endpoint);
}

template <bool realtime>
void handle_status(nova_endpoint const & endpoint)
{
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(status_perform, endpoint));
}

void handle_dumpOSC(received_message const & message)
{
    int val = first_arg_as_int(message);
    val = min (1, val);    /* we just support one way of dumping osc messages */

    instance->dumpOSC(val);     /* thread-safe */
}

void sync_perform(osc::int32 id, nova_endpoint const & endpoint)
{
    char buffer[128];
    osc::OutboundPacketStream p(buffer, 128);
    p << osc::BeginMessage("/synced")
      << id
      << osc::EndMessage;

    instance->send(p.Data(), p.Size(), endpoint);
}

template <bool realtime>
void handle_sync(received_message const & message, nova_endpoint const & endpoint)
{
    int id = first_arg_as_int(message);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(sync_perform, id, endpoint));
}

void handle_clearSched(void)
{
    instance->clear_scheduled_bundles();
}

void handle_error(received_message const & message)
{
    int val = first_arg_as_int(message);

    instance->set_error_posting(val);     /* thread-safe */
}

void handle_unhandled_message(received_message const & msg)
{
    cerr << "unhandled message " << msg.AddressPattern() << endl;
}

sc_synth * add_synth(const char * name, int node_id, int action, int target_id)
{
    if (!check_node_id(node_id))
        return 0;

    server_node * target = find_node(target_id);
    if (target == NULL)
        return NULL;

    node_position_constraint pos = make_pair(target, node_position(action));
    abstract_synth * synth = instance->add_synth(name, node_id, pos);

    last_generated = node_id;
    return static_cast<sc_synth*>(synth);
}

/* extract float or int32 as float from argument iterator */
inline float extract_float_argument(osc::ReceivedMessageArgumentIterator const & it)
{
    if (it->IsFloat())
        return it->AsFloatUnchecked();
    if (it->IsInt32())
        return float(it->AsInt32Unchecked());
    if (it->IsInt64())
        return float(it->AsInt64Unchecked());

    throw std::runtime_error("type cannot be converted to float");
}

inline void verify_argument(osc::ReceivedMessageArgumentIterator const & it,
                            osc::ReceivedMessageArgumentIterator const & end)
{
    if (it == end)
        throw std::runtime_error("unexpected end of argument list");
}


template <typename control_id_type>
void set_control_array(server_node * node, control_id_type control, osc::ReceivedMessageArgumentIterator & it)
{
    size_t array_size = it->ArraySize();
    sized_array<float, rt_pool_allocator<float> > control_array(array_size);

    ++it; // advance to first element
    for (size_t i = 0; i != array_size; ++i)
        control_array[i] = extract_float_argument(it++);
    assert(it->IsArrayEnd());
    ++it; // skip array end

    node->set(control, array_size, control_array.c_array());
}

/* set control values of node from string/float or int/float pair */
void set_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 index = it->AsInt32Unchecked(); ++it;
        if (it->IsArrayStart())
            set_control_array(node, index, it);
        else {
            float value = extract_float_argument(it++);
            node->set(index, value);
        }
    }
    else if (it->IsString()) {
        const char * str = it->AsString(); ++it;

        if (it->IsArrayStart())
            set_control_array(node, str, it);
        else {
            float value = extract_float_argument(it++);
            node->set(str, value);
        }
    } else
        throw runtime_error("invalid argument");
}

void handle_s_new(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator args = msg.ArgumentsBegin();

    const char * def_name = args->AsString(); ++args;
    int32_t id = args->AsInt32(); ++args;

    if (id == -1)
        id = instance->generate_node_id();

    int32_t action, target;

    if (args != msg.ArgumentsEnd()) {
        action = args->AsInt32(); ++args;
    } else
        action = 0;

    if (args != msg.ArgumentsEnd()) {
        target = args->AsInt32(); ++args;
    } else
        target = 0;

    sc_synth * synth = add_synth(def_name, id, action, target);

    if (synth == NULL)
        return;

    while(args != msg.ArgumentsEnd())
    {
        try {
            set_control(synth, args);
        }
        catch(std::exception & e)
        {
            cout << "Exception during /s_new handler: " << e.what() << endl;
        }
    }
}


void insert_group(int node_id, int action, int target_id)
{
    if (node_id == -1)
        node_id = instance->generate_node_id();
    else if (!check_node_id(node_id))
        return;

    server_node * target = find_node(target_id);

    if (!target)
        return;

    node_position_constraint pos = make_pair(target, node_position(action));

    instance->add_group(node_id, pos);
    last_generated = node_id;
}

void handle_g_new(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id, action, target;
        args >> id >> action >> target;

        insert_group(id, action, target);
    }
}

void handle_g_head(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id, target;
        args >> id >> target;

        insert_group(id, head, target);
    }
}

void handle_g_tail(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id, target;
        args >> id >> target;

        insert_group(id, tail, target);
    }
}

void handle_g_freeall(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id;
        args >> id;

        abstract_group * group = find_group(id);
        if (!group)
            continue;

        bool success = instance->group_free_all(group);

        if (!success)
            cerr << "/g_freeAll failue" << endl;
    }
}

void handle_g_deepFree(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id;
        args >> id;

        abstract_group * group = find_group(id);
        if (!group)
            continue;

        bool success = instance->group_free_deep(group);

        if (!success)
            cerr << "/g_freeDeep failue" << endl;
    }
}

void g_query_tree_fill_node(osc::OutboundPacketStream & p, bool flag, server_node const & node)
{
    p << osc::int32(node.id());
    if (node.is_synth())
        p << -1;
    else
        p << osc::int32(static_cast<abstract_group const &>(node).child_count());

    if (node.is_synth()) {
        sc_synth const & scsynth = static_cast<sc_synth const&>(node);
        p << scsynth.prototype_name();

        if (flag) {
            osc::int32 controls = scsynth.mNumControls;
            p << controls;

            for (int i = 0; i != controls; ++i) {
                p << osc::int32(i); /** \todo later we can return symbols */

                if (scsynth.mMapControls[i] != (scsynth.mControls+i)) {
                    /* we use a bus mapping */
                    int bus = (scsynth.mMapControls[i]) - (scsynth.mNode.mWorld->mControlBus);
                    char str[10];
                    sprintf(str, "s%d", bus);
                    p << str;
                }
                else
                    p << scsynth.mControls[i];
            }
        }
    }
}

template <bool realtime>
void g_query_tree(int node_id, bool flag, nova_endpoint const & endpoint)
{
    server_node * node = find_node(node_id);
    if (!node || node->is_synth())
        return;

    abstract_group * group = static_cast<abstract_group*>(node);

    size_t max_msg_size = 1<<16;
    for(;;) {
        try {
            if (max_msg_size > 1<<22)
                return;

            sized_array<char, rt_pool_allocator<char> > data(max_msg_size);

            osc::OutboundPacketStream p(data.c_array(), max_msg_size);
            p << osc::BeginMessage("/g_queryTree.reply")
              << (flag ? 1 : 0)
              << node_id
              << osc::int32(group->child_count());

            group->apply_on_children(boost::bind(g_query_tree_fill_node, boost::ref(p), flag, _1));
            p << osc::EndMessage;

            movable_array<char> message(p.Size(), data.c_array());
            cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
            return;
        }
        catch(...)
        {
            max_msg_size *= 2; /* if we run out of memory, retry with doubled memory resources */
        }
    }
}

template <bool realtime>
void handle_g_queryTree(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        try {
            osc::int32 id, flag;
            args >> id >> flag;
            g_query_tree<realtime>(id, flag, endpoint);
        }
        catch (std::exception & e) {
            cerr << e.what() << endl;
        }
    }
}

void fill_spaces(int level)
{
    for (int i = 0; i != level*3; ++i)
        cout << ' ';
}

void g_dump_node(server_node & node, bool flag, int level)
{
    using namespace std;
    fill_spaces(level);

    if (node.is_synth()) {
        abstract_synth const & synth = static_cast<abstract_synth const &>(node);
        cout << synth.id() << " " << synth.prototype_name() << endl;

        if (flag) {
            /* dump controls */
        }
    } else {
        abstract_group & group = static_cast<abstract_group &>(node);
        cout << group.id();

        if (group.is_parallel())
            cout << " parallel group";
        else
            cout << " group";
        cout << endl;
        group.apply_on_children(boost::bind(g_dump_node, _1, flag, level + 1));
    }
}

void g_dump_tree(int id, bool flag)
{
    std::cout << "NODE TREE Group " << id << std::endl;
    server_node * node = find_node(id);
    if (!node)
        return;

    g_dump_node(*node, flag, 1);
}

void handle_g_dumpTree(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        try {
            osc::int32 id, flag;
            args >> id >> flag;
            g_dump_tree(id, flag);
        }
        catch (std::exception & e) {
            cerr << e.what() << endl;
        }
    }
}

void handle_n_free(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        try {
            osc::int32 id;
            args >> id;

            server_node * node = find_node(id);
            if (!node)
                continue;

            instance->free_node(node);
        }
        catch (std::exception & e) {
            cerr << e.what() << endl;
        }
    }
}

/** macro to define an os command handler with a starting node id
 *
 *  it is mainly intended as decorator to avoid duplicate error handling code
 */
#define HANDLE_N_DECORATOR(cmd, function)                               \
void handle_n_##cmd(received_message const & msg)                       \
{                                                                       \
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();     \
    osc::int32 id = it->AsInt32(); ++it;                                \
                                                                        \
    server_node * node = find_node(id);                                 \
    if(!node)                                                           \
        return;                                                         \
                                                                        \
    while(it != msg.ArgumentsEnd())                                     \
    {                                                                   \
        try                                                             \
        {                                                               \
            function(node, it);                                         \
        }                                                               \
        catch(std::exception & e)                                       \
        {                                                               \
            cout << "Exception during /n_" #cmd "handler: " << e.what() << endl; \
            return;                                                     \
        }                                                               \
    }                                                                   \
}

HANDLE_N_DECORATOR(set, set_control)

void set_control_n(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 index = it->AsInt32Unchecked(); ++it;
        osc::int32 count = it->AsInt32(); ++it;

        for (int i = 0; i != count; ++i)
            node->set(index + i, extract_float_argument(it++));
    }
    else if (it->IsString()) {
        const char * str = it->AsStringUnchecked(); ++it;
        osc::int32 count = it->AsInt32(); ++it;

        sized_array<float> values(count);
        for (int i = 0; i != count; ++i)
            values[i] = extract_float_argument(it++);

        node->set(str, count, values.c_array());
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(setn, set_control_n)

void fill_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 index = it->AsInt32Unchecked(); ++it;
        osc::int32 count = it->AsInt32(); ++it;
        float value = extract_float_argument(it++);

        for (int i = 0; i != count; ++i)
            node->set(index + i, value);
    }
    else if (it->IsString()) {
        const char * str = it->AsStringUnchecked(); ++it;
        osc::int32 count = it->AsInt32(); ++it;
        float value = extract_float_argument(it++);

        sized_array<float> values(count);
        for (int i = 0; i != count; ++i)
            values[i] = value;

        node->set(str, count, values.c_array());
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(fill, fill_control)


template <typename slot_type>
void handle_n_map_group(server_node & node, slot_type slot, int control_bus_index)
{
    if (node.is_synth())
        static_cast<sc_synth&>(node).map_control_bus(slot, control_bus_index);
    else
        static_cast<abstract_group&>(node).apply_on_children(boost::bind(handle_n_map_group<slot_type>, _1,
                                                                         slot, control_bus_index));
}

void map_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 control_index = it->AsInt32Unchecked(); ++it;
        osc::int32 control_bus_index = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_bus(control_index, control_bus_index);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_map_group<slot_index_t>, _1,
                                                                              control_index, control_bus_index));
    }
    else if (it->IsString()) {
        const char * control_name = it->AsStringUnchecked(); ++it;
        osc::int32 control_bus_index = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_bus(control_name, control_bus_index);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_map_group<const char*>, _1,
                                                                              control_name, control_bus_index));
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(map, map_control)


template <typename slot_type>
void handle_n_mapn_group(server_node & node, slot_type slot, int control_bus_index, int count)
{
    if (node.is_synth())
        static_cast<sc_synth&>(node).map_control_buses(slot, control_bus_index, count);
    else
        static_cast<abstract_group&>(node).apply_on_children(boost::bind(handle_n_mapn_group<slot_type>, _1,
                                                                         slot, control_bus_index, count));
}

void mapn_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 control_index = it->AsInt32Unchecked(); ++it;
        osc::int32 control_bus_index = it->AsInt32(); ++it;
        osc::int32 count = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_buses(control_index, control_bus_index, count);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapn_group<slot_index_t>, _1,
                                                                                control_index, control_bus_index, count));
    }
    else if (it->IsString()) {
        const char * control_name = it->AsStringUnchecked(); ++it;
        osc::int32 control_bus_index = it->AsInt32(); ++it;
        osc::int32 count = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_buses(control_name, control_bus_index, count);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapn_group<const char*>, _1,
                                                                              control_name, control_bus_index, count));
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(mapn, mapn_control)

template <typename slot_type>
void handle_n_mapa_group(server_node & node, slot_type slot, int audio_bus_index)
{
    if (node.is_synth())
        static_cast<sc_synth&>(node).map_control_bus_audio(slot, audio_bus_index);
    else
        static_cast<abstract_group&>(node).apply_on_children(boost::bind(handle_n_mapa_group<slot_type>, _1,
                                                                         slot, audio_bus_index));
}

void mapa_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        osc::int32 control_index = it->AsInt32Unchecked(); ++it;
        osc::int32 audio_bus_index = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_bus_audio(control_index, audio_bus_index);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapa_group<slot_index_t>, _1,
                                                                                control_index, audio_bus_index));
    }
    else if (it->IsString()) {
        const char * control_name = it->AsStringUnchecked(); ++it;
        osc::int32 audio_bus_index = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_bus_audio(control_name, audio_bus_index);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapa_group<const char *>, _1,
                                                                              control_name, audio_bus_index));
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(mapa, mapa_control)

template <typename slot_type>
void handle_n_mapan_group(server_node & node, slot_type slot, int audio_bus_index, int count)
{
    if (node.is_synth())
        static_cast<sc_synth&>(node).map_control_buses_audio(slot, audio_bus_index, count);
    else
        static_cast<abstract_group&>(node).apply_on_children(boost::bind(handle_n_mapan_group<slot_type>, _1,
                                                                         slot, audio_bus_index, count));
}

void mapan_control(server_node * node, osc::ReceivedMessageArgumentIterator & it)
{
    if (it->IsInt32()) {
        if (it->IsInt32()) {
            osc::int32 control_index = it->AsInt32Unchecked(); ++it;
            osc::int32 audio_bus_index = it->AsInt32(); ++it;
            osc::int32 count = it->AsInt32(); ++it;

            if (node->is_synth()) {
                sc_synth * synth = static_cast<sc_synth*>(node);
                synth->map_control_buses_audio(control_index, audio_bus_index, count);
            }
            else
                static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapan_group<slot_index_t>, _1,
                                                                                  control_index, audio_bus_index, count));
    }
    else if (it->IsString()) {
        const char * control_name = it->AsStringUnchecked(); ++it;
        osc::int32 audio_bus_index = it->AsInt32(); ++it;
        osc::int32 count = it->AsInt32(); ++it;

        if (node->is_synth()) {
            sc_synth * synth = static_cast<sc_synth*>(node);
            synth->map_control_buses_audio(control_name, audio_bus_index, count);
        }
        else
            static_cast<abstract_group*>(node)->apply_on_children(boost::bind(handle_n_mapan_group<const char *>, _1,
                                                                              control_name, audio_bus_index, count));
        }
    } else
        throw runtime_error("invalid argument");
}

HANDLE_N_DECORATOR(mapan, mapan_control)

void handle_n_before(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_a, node_b;
        args >> node_a >> node_b;

        server_node * a = find_node(node_a);
        server_node * b = find_node(node_b);

        abstract_group * a_parent = a->get_parent();
        abstract_group * b_parent = b->get_parent();

        /** \todo this can be optimized if a_parent == b_parent */
        a_parent->remove_child(a);
        b_parent->add_child(a, make_pair(b_parent, before));
    }
}

void handle_n_after(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_a, node_b;
        args >> node_a >> node_b;

        server_node * a = find_node(node_a);
        server_node * b = find_node(node_b);

        abstract_group * a_parent = a->get_parent();
        abstract_group * b_parent = b->get_parent();

        /** \todo this can be optimized if a_parent == b_parent */
        a_parent->remove_child(a);
        b_parent->add_child(a, make_pair(b_parent, after));
    }
}

void handle_n_query(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_id;
        args >> node_id;

        server_node * node = find_node(node_id);
        if (!node)
            continue;

        char buffer[128]; // 128 byte should be enough
        osc::OutboundPacketStream p(buffer, 128);
        p << osc::BeginMessage("/n_info");
        fill_notification(node, p);

        movable_array<char> message(p.Size(), p.Data());
        cmd_dispatcher<true>::fire_system_callback(boost::bind(send_udp_message, message, endpoint));
    }
}

void handle_n_order(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    osc::int32 action, target_id;
    args >> action >> target_id;

    server_node * target = find_node(target_id);

    if (target == NULL)
        return;

    abstract_group * target_parent;
    if (action == before ||
        action == after)
        target_parent = target->get_parent();
    else {
        if (target->is_synth())
            throw std::runtime_error("invalid argument for n_order: argument is no synth");
        target_parent = static_cast<abstract_group*>(target);
    }

    while (!args.Eos())
    {
        osc::int32 node_id;
        args >> node_id;

        server_node * node = find_node(node_id);
        if (node == NULL)
            continue;

        abstract_group * node_parent = node->get_parent();

        /** \todo this can be optimized if node_parent == target_parent */
        node_parent->remove_child(node);
        if (action == before ||
            action == after)
            target_parent->add_child(node, make_pair(target, node_position(action)));
        else
            target_parent->add_child(node, node_position(action));
    }
    instance->update_dsp_queue();
}


void handle_n_run(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_id, run_flag;
        args >> node_id >> run_flag;

        server_node * node = find_node(node_id);
        if(!node)
            continue;

        if (run_flag)
            instance->node_resume(node);
        else
            instance->node_pause(node);
    }
}

void enable_tracing(server_node & node)
{
    if (node.is_synth()) {
        sc_synth & synth = static_cast<sc_synth&>(node);
        synth.enable_tracing();
    } else {
        abstract_group & group = static_cast<abstract_group&>(node);
        group.apply_on_children(enable_tracing);
    }
}

void handle_n_trace(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_id;
        args >> node_id;

        server_node * node = find_node(node_id);
        if (!node)
            continue;

        enable_tracing(*node);
    }
}


void handle_s_noid(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 node_id;
        args >> node_id;
        instance->synth_reassign_id(node_id);
    }
}

int32_t get_control_index(sc_synth * s, osc::ReceivedMessageArgumentIterator & it, osc::OutboundPacketStream & p)
{
    int32_t control;
    if (it->IsInt32())
    {
        control = it->AsInt32Unchecked(); ++it;
        p << control;
    }
    else if (it->IsString())
    {
        const char * control_str = it->AsStringUnchecked(); ++it;
        control = s->resolve_slot(control_str);
        p << control_str;
    }
    else if (it->IsSymbol())
    {
        const char * control_str = it->AsSymbolUnchecked(); ++it;
        control = s->resolve_slot(control_str);
        p << osc::Symbol(control_str);
    }
    else
        throw std::runtime_error("wrong argument type");
    return control;
}

template <bool realtime>
void handle_s_get(received_message const & msg, size_t msg_size, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();

    if (!it->IsInt32())
        throw std::runtime_error("wrong argument type");

    int32_t node_id = it->AsInt32Unchecked(); ++it;

    server_node * node = find_node(node_id);
    if (!node || !node->is_synth())
        throw std::runtime_error("node is not a synth");

    sc_synth * s = static_cast<sc_synth*>(node);

    size_t alloc_size = msg_size + sizeof(float) * (msg.ArgumentCount()-1) + 128;

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/n_set")
      << node_id;

    while (it != msg.ArgumentsEnd())
    {
        int32_t control = get_control_index(s, it, p);
        p << s->get(control);
    }
    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}

template <bool realtime>
void handle_s_getn(received_message const & msg, size_t msg_size, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();

    if (!it->IsInt32())
        throw std::runtime_error("wrong argument type");

    int32_t node_id = it->AsInt32Unchecked(); ++it;

    server_node * node = find_node(node_id);
    if (!node || !node->is_synth())
        throw std::runtime_error("node is not a synth");

    sc_synth * s = static_cast<sc_synth*>(node);

    /* count argument values */
    size_t argument_count = 0;
    for (osc::ReceivedMessageArgumentIterator local = it; local != msg.ArgumentsEnd(); ++local)
    {
        ++local; /* skip control */
        if (local == msg.ArgumentsEnd())
            break;
        if (!it->IsInt32())
            throw std::runtime_error("invalid count");
        argument_count += it->AsInt32Unchecked(); ++it;
    }

    size_t alloc_size = msg_size + sizeof(float) * (argument_count) + 128;

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/n_setn")
      << node_id;

    while (it != msg.ArgumentsEnd())
    {
        int32_t control = get_control_index(s, it, p);

        if (!it->IsInt32())
            throw std::runtime_error("integer argument expected");

        int32_t control_count = it->AsInt32Unchecked(); ++it;
        if (control_count < 0)
            break;

        for (int i = 0; i != control_count; ++i)
            p << s->get(control + i);
    }
    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}


/** wrapper class for osc completion message
 */
struct completion_message
{
    /** constructor should only be used from the real-time thread */
    completion_message(size_t size, const void * data):
        size_(size)
    {
        if (size)
        {
            data_ = system_callback::allocate(size);
            memcpy(data_, data, size);
        }
    }

    /** default constructor creates uninitialized object */
    completion_message(void):
        size_(0)
    {}

    /** copy constructor has move semantics!!! */
    completion_message(completion_message const & rhs)
    {
        size_ = rhs.size_;
        data_ = rhs.data_;
        const_cast<completion_message&>(rhs).size_ = 0;
    }

    ~completion_message(void)
    {
        if (size_)
            system_callback::deallocate(data_);
    }

    /** handle package in the rt thread
     *  not to be called from the rt thread
     */
    void trigger_async(nova_endpoint const & endpoint)
    {
        if (size_)
        {
            sc_osc_handler::received_packet * p =
                sc_osc_handler::received_packet::alloc_packet((char*)data_, size_, endpoint);
            instance->add_sync_callback(p);
        }
    }

    /** handle package directly
     *  only to be called from the rt thread
     */
    void handle(nova_endpoint const & endpoint)
    {
        if (size_)
            instance->handle_packet((char*)data_, size_, endpoint);
    }

    size_t size_;
    void * data_;
};

completion_message extract_completion_message(osc::ReceivedMessageArgumentStream & args)
{
    osc::Blob blob(0, 0);

    if (!args.Eos()) {
        try {
            args >> blob;
        }
        catch (osc::WrongArgumentTypeException & e)
        {}
    }

    return completion_message (blob.size, blob.data);
}

completion_message extract_completion_message(osc::ReceivedMessageArgumentIterator & it)
{
    const void * data = 0;
    unsigned long length = 0;

    if (it->IsBlob())
        it->AsBlobUnchecked(data, length);
    ++it;
    return completion_message(length, data);
}


template <bool realtime>
void b_alloc_2_rt(uint32_t index, completion_message & msg, sample * free_buf, nova_endpoint const & endpoint);
void b_alloc_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);

template <bool realtime>
void b_alloc_1_nrt(uint32_t index, uint32_t frames, uint32_t channels, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sample * free_buf = sc_factory->get_nrt_mirror_buffer(index);
    sc_factory->allocate_buffer(index, frames, channels);
    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_alloc_2_rt<realtime>, index, msg, free_buf, endpoint));
}

template <bool realtime>
void b_alloc_2_rt(uint32_t index, completion_message & msg, sample * free_buf, nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_alloc_3_nrt, index, free_buf, endpoint));
}

void b_alloc_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    free_aligned(free_buf);
    send_done_message(endpoint, "/b_alloc", index);
}

template <bool realtime>
void handle_b_alloc(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    osc::int32 index, frames, channels;

    args >> index >> frames;

    if (!args.Eos())
        args >> channels;
    else
        channels = 1;

    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_alloc_1_nrt<realtime>, index, frames,
                                                           channels, message, endpoint));
}

template <bool realtime>
void b_free_1_nrt(uint32_t index, completion_message & msg, nova_endpoint const & endpoint);
template <bool realtime>
void b_free_2_rt(uint32_t index, sample * free_buf, completion_message & msg, nova_endpoint const & endpoint);
void b_free_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);

template <bool realtime>
void b_free_1_nrt(uint32_t index, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sample * free_buf = sc_factory->get_nrt_mirror_buffer(index);
    sc_factory->free_buffer(index);
    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_free_2_rt<realtime>,
                                                           index, free_buf, msg, endpoint));
}

template <bool realtime>
void b_free_2_rt(uint32_t index, sample * free_buf, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_free_3_nrt, index, free_buf, endpoint));
    msg.handle(endpoint);
}

void b_free_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    free_aligned(free_buf);
    send_done_message(endpoint, "/b_free", index);
}


template <bool realtime>
void handle_b_free(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    osc::int32 index;
    args >> index;

    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_free_1_nrt<realtime>, index, message, endpoint));
}

template <bool realtime>
void b_allocRead_2_rt(uint32_t index, completion_message & msg, sample * free_buf, nova_endpoint const & endpoint);
void b_allocRead_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);

template <bool realtime>
void b_allocRead_1_nrt(uint32_t index, movable_string & filename, uint32_t start, uint32_t frames, completion_message & msg,
                       nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sample * free_buf = sc_factory->get_nrt_mirror_buffer(index);
    int error = sc_factory->buffer_read_alloc(index, filename.c_str(), start, frames);
    if (!error)
        cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_allocRead_2_rt<realtime>, index, msg, free_buf, endpoint));
    else
        /* post nice error message */;
}

template <bool realtime>
void b_allocRead_2_rt(uint32_t index, completion_message & msg, sample * free_buf,
                      nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_allocRead_3_nrt, index, free_buf, endpoint));
}

void b_allocRead_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    free_aligned(free_buf);
    send_done_message(endpoint, "/b_allocRead", index);
}

template <bool realtime>
void handle_b_allocRead(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    osc::int32 index;
    const char * filename;

    osc::int32 start = 0;
    osc::int32 frames = 0;

    args >> index >> filename;

    if (!args.Eos())
        args >> start;

    if (!args.Eos())
        args >> frames;

    completion_message message = extract_completion_message(args);

    movable_string fname(filename);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_allocRead_1_nrt<realtime>, index,
                                                               fname, start, frames, message, endpoint));
}

template <bool realtime>
void b_allocReadChannel_2_rt(uint32_t index, completion_message & msg, sample * free_buf,
                             nova_endpoint const & endpoint);
void b_allocReadChannel_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);

template <bool realtime>
void b_allocReadChannel_1_nrt(uint32_t index, movable_string const & filename, uint32_t start, uint32_t frames,
                              movable_array<uint32_t> const & channels, completion_message & msg,
                              nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sample * free_buf = sc_factory->get_nrt_mirror_buffer(index);
    int error = sc_factory->buffer_alloc_read_channels(index, filename.c_str(), start, frames,
                                                        channels.size(), channels.data());
    if (!error)
        cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_allocReadChannel_2_rt<realtime>,
                                                               index, msg, free_buf, endpoint));
}

template <bool realtime>
void b_allocReadChannel_2_rt(uint32_t index, completion_message & msg, sample * free_buf,
                             nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_allocReadChannel_3_nrt,
                                                               index, free_buf, endpoint));
}

void b_allocReadChannel_3_nrt(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    free_aligned(free_buf);
    send_done_message(endpoint, "/b_allocReadChannel", index);
}


template <bool realtime>
void handle_b_allocReadChannel(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator arg = msg.ArgumentsBegin();

    osc::int32 index = arg->AsInt32(); arg++;
    const char * filename = arg->AsString(); arg++;

    osc::int32 start = arg->AsInt32(); arg++;
    size_t frames = arg->AsInt32(); arg++;

    size_t channel_args = msg.ArgumentCount() - 4; /* we already consumed 4 elements */

    size_t channel_count = 0;
    sized_array<uint, rt_pool_allocator<uint> > channels(channel_args);

    for (uint i = 0; i != channel_args - 1; ++i) // sclang fromats the last completion message as int, so we skip the last element
    {
        if (arg->IsInt32()) {
            channels[i] = arg->AsInt32Unchecked(); arg++;
            ++channel_count;
        }
    }

    /* we reached the message blob */
    completion_message message = extract_completion_message(arg);

    movable_array<uint32_t> channel_mapping(channel_count, channels.c_array());
    movable_string fname(filename);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_allocReadChannel_1_nrt<realtime>,
                                                           index, fname, start, frames, channel_mapping,
                                                           message, endpoint));
}

const char * b_write = "/b_write";

template <bool realtime>
void b_write_nrt_1(uint32_t index, movable_string const & filename, movable_string const & header_format,
                   movable_string const & sample_format, uint32_t start, uint32_t frames, bool leave_open,
                   completion_message & msg, nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sc_factory->buffer_write(index, filename.c_str(), header_format.c_str(), sample_format.c_str(), start, frames, leave_open);
    msg.trigger_async(endpoint);
    cmd_dispatcher<realtime>::fire_done_message(endpoint, b_write, index);
}

void fire_b_write_exception(void)
{
    throw std::runtime_error("wrong arguments for /b_allocReadChannel");
}

template <bool realtime>
void handle_b_write(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator arg = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();

    /* required args */
    osc::int32 index = arg->AsInt32(); arg++;
    const char * filename = arg->AsString(); arg++;
    const char * header_format = arg->AsString(); arg++;
    const char * sample_format = arg->AsString(); arg++;

    /* optional args */
    osc::int32 frames = -1;
    osc::int32 start = 0;
    osc::int32 leave_open = 0;

    completion_message message;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_write_exception();
        frames = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_write_exception();
        start = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_write_exception();
        leave_open = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end)
        message = extract_completion_message(arg);

fire_callback:
    movable_string fname(filename);
    movable_string header_f(header_format);
    movable_string sample_f(sample_format);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_write_nrt_1<realtime>, index, fname, header_f, sample_f,
                                               start, frames, bool(leave_open), message, endpoint));
}

template <bool realtime>
void b_read_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint);

template <bool realtime>
void b_read_nrt_1(uint32_t index, movable_string & filename, uint32_t start_file, uint32_t frames,
                  uint32_t start_buffer, bool leave_open, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_ugen_factory::buffer_lock_t buffer_lock(sc_factory->buffer_guard(index));
    sc_factory->buffer_read(index, filename.c_str(), start_file, frames, start_buffer, leave_open);
    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_read_rt_2<realtime>, index, msg, endpoint));
}

const char * b_read = "/b_read";
template <bool realtime>
void b_read_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_done_message(endpoint, b_read, index);
}

void fire_b_read_exception(void)
{
    throw std::runtime_error("wrong arguments for /b_read");
}

template <bool realtime>
void handle_b_read(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator arg = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();

    /* required args */
    osc::int32 index = arg->AsInt32(); arg++;
    const char * filename = arg->AsString(); arg++;

    /* optional args */
    osc::int32 start_file = 0;
    osc::int32 frames = -1;
    osc::int32 start_buffer = 0;
    osc::int32 leave_open = 0;

    completion_message message;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        start_file = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        frames = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        start_buffer = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        leave_open = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end)
        message = extract_completion_message(arg);

fire_callback:
    movable_string fname(filename);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_read_nrt_1<realtime>, index, fname,
                                                               start_file, frames, start_buffer,
                                                               bool(leave_open), message, endpoint));
}


template <bool realtime>
void b_readChannel_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint);

template <bool realtime>
void b_readChannel_nrt_1(uint32_t index, movable_string & filename, uint32_t start_file, uint32_t frames,
                         uint32_t start_buffer, bool leave_open, movable_array<uint32_t> & channel_map,
                         completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->buffer_read_channel(index, filename.c_str(), start_file, frames, start_buffer, leave_open,
                                     channel_map.size(), channel_map.data());
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_readChannel_rt_2<realtime>, index, msg, endpoint));
}

const char * b_readChannel = "/b_readChannel";
template <bool realtime>
void b_readChannel_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_done_message(endpoint, b_readChannel, index);
}

void fire_b_readChannel_exception(void)
{
    throw std::runtime_error("wrong arguments for /b_readChannel");
}

template <bool realtime>
void handle_b_readChannel(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator arg = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();

    /* required args */
    osc::int32 index = arg->AsInt32(); arg++;
    const char * filename = arg->AsString(); arg++;

    /* optional args */
    osc::int32 start_file = 0;
    osc::int32 frames = -1;
    osc::int32 start_buffer = 0;
    osc::int32 leave_open = 0;

    sized_array<uint32_t, rt_pool_allocator<uint32_t> > channel_mapping(int32_t(msg.ArgumentCount())); /* larger than required */
    uint32_t channel_count = 0;

    completion_message message;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        start_file = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_read_exception();
        frames = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_write_exception();
        start_buffer = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    if (arg != end) {
        if (!arg->IsInt32())
            fire_b_write_exception();
        leave_open = arg->AsInt32Unchecked(); arg++;
    }
    else
        goto fire_callback;

    while (arg != end)
    {
        if (arg->IsBlob()) {
            message = extract_completion_message(arg);
            goto fire_callback;
        }
        else if (arg->IsInt32()) {
            channel_mapping[channel_count] = arg->AsInt32Unchecked();
            ++arg;
        }
        else
            fire_b_readChannel_exception();
    }

fire_callback:
    movable_string fname(filename);
    movable_array<uint32_t> channel_map(channel_count, channel_mapping.c_array());

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_readChannel_nrt_1<realtime>, index, fname,
                                                           start_file, frames, start_buffer,
                                                           bool(leave_open), channel_map, message, endpoint));
}


template <bool realtime>
void b_zero_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint);

template <bool realtime>
void b_zero_nrt_1(uint32_t index, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->buffer_zero(index);
    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_zero_rt_2<realtime>, index, msg, endpoint));
}

const char * b_zero = "/b_zero";
template <bool realtime>
void b_zero_rt_2(uint32_t index, completion_message & msg, nova_endpoint const & endpoint)
{
    sc_factory->increment_write_updates(index);
    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_done_message(endpoint, b_zero, index);
}

template <bool realtime>
void handle_b_zero(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    osc::int32 index;
    args >> index;
    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_zero_nrt_1<realtime>, index, message, endpoint));
}

void handle_b_set(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();
    verify_argument(it, end);
    osc::int32 buffer_index = it->AsInt32(); ++it;

    buffer_wrapper::sample_t * data = sc_factory->get_buffer(buffer_index);

    while (it != end) {
        osc::int32 index = it->AsInt32(); ++it;
        float value = extract_float_argument(it++);

        data[index] = value;
    }
}

void handle_b_setn(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();
    verify_argument(it, end);
    osc::int32 buffer_index = it->AsInt32(); ++it;

    buffer_wrapper::sample_t * data = sc_factory->get_buffer(buffer_index);

    while (it != end) {
        osc::int32 index = it->AsInt32(); ++it;
        verify_argument(it, end);
        osc::int32 samples = it->AsInt32(); ++it;

        for (int i = 0; i != samples; ++i) {
            verify_argument(it, end);
            float value = extract_float_argument(it++);
            data[index+i] = value;
        }
    }
}

void handle_b_fill(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();
    osc::ReceivedMessageArgumentIterator end = msg.ArgumentsEnd();
    verify_argument(it, end);
    osc::int32 buffer_index = it->AsInt32(); ++it;

    buffer_wrapper::sample_t * data = sc_factory->get_buffer(buffer_index);

    while (it != end) {
        osc::int32 index = it->AsInt32(); ++it;
        verify_argument(it, end);
        osc::int32 samples = it->AsInt32(); ++it;
        verify_argument(it, end);
        float value = extract_float_argument(it++);

        for (int i = 0; i != samples; ++i)
            data[index] = value;
    }
}

template <bool realtime>
void handle_b_query(received_message const & msg, nova_endpoint const & endpoint)
{
    const size_t elem_size = 3*sizeof(int) * sizeof(float);

    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    size_t arg_count = msg.ArgumentCount();

    size_t size = elem_size * arg_count + 128; /* should be more than required */
    sized_array<char, rt_pool_allocator<char> > data(size);

    osc::OutboundPacketStream p(data.c_array(), size);
    p << osc::BeginMessage("/b_info");

    while (!args.Eos()) {
        osc::int32 buffer_index;
        args >> buffer_index;

        SndBuf * buf = sc_factory->get_buffer_struct(buffer_index);

        p << buffer_index
          << osc::int32(buf->frames)
          << osc::int32(buf->channels)
          << float (buf->samplerate);
    }

    p << osc::EndMessage;

    movable_array<char> message(p.Size(), data.c_array());

    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}

void b_close_nrt_1(uint32_t index)
{
    sc_factory->buffer_close(index);
}

template <bool realtime>
void handle_b_close(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    osc::int32 index;

    args >> index;

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_close_nrt_1, index));
}

template <bool realtime>
void handle_b_get(received_message const & msg, nova_endpoint const & endpoint)
{
    const size_t elem_size = sizeof(int) * sizeof(float);
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    const size_t index_count = msg.ArgumentCount() - 1;
    const size_t alloc_size = index_count * elem_size + 128; /* hopefully enough */

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::int32 buffer_index;
    args >> buffer_index;

    const SndBuf * buf = sc_factory->get_buffer_struct(buffer_index);
    const sample * data = buf->data;
    const int max_sample = buf->frames * buf->channels;

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/b_set")
      << buffer_index;

    while (!args.Eos())
    {
        osc::int32 index;
        args >> index;
        p << index;

        if (index < max_sample)
            p << data[index];
        else
            p << 0.f;
    }

    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}

template<typename Alloc>
struct getn_data
{
    getn_data(int start, int count, const float * data):
        start_index_(start), data_(count)
    {
        data_.reserve(count);
        for (int i = 0; i != count; ++i)
            data_[i] = data[i];
    }

    int start_index_;
    std::vector<float, Alloc> data_;
};

template <bool realtime>
void handle_b_getn(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    typedef getn_data<rt_pool_allocator<float> > getn_data;
    std::vector<getn_data, rt_pool_allocator<getn_data> > return_data;

    osc::int32 buffer_index;
    args >> buffer_index;

    const SndBuf * buf = sc_factory->get_buffer_struct(buffer_index);
    const sample * data = buf->data;
    const int max_sample = buf->frames * buf->channels;

    while (!args.Eos())
    {
        osc::int32 index, sample_count;
        args >> index >> sample_count;

        if (index + sample_count <= max_sample)
            return_data.push_back(getn_data(index, sample_count, data + index));
    }

    size_t alloc_size = 128;
    for (size_t i = 0; i != return_data.size(); ++i)
        alloc_size += return_data[i].data_.size() * (sizeof(float) + sizeof(int)) + 2*sizeof(int);

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/b_setn")
      << buffer_index;

    for (size_t i = 0; i != return_data.size(); ++i) {
        p << osc::int32(return_data[i].start_index_)
          << osc::int32(return_data[i].data_.size());

        for (size_t j = 0; j != return_data[i].data_.size(); ++j)
            p << return_data[i].data_[j];
    }

    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}


template <bool realtime>
void b_gen_rt_2(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);
void b_gen_nrt_3(uint32_t index, sample * free_buf, nova_endpoint const & endpoint);

template <bool realtime>
void b_gen_nrt_1(movable_array<char> & message, nova_endpoint const & endpoint)
{
    sc_msg_iter msg(message.size(), (char*)message.data());

    int index = msg.geti();
    const char * generator = (const char*)msg.gets4();
    if (!generator)
        return;

    sample * free_buf = sc_factory->buffer_generate(index, generator, msg);
    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(b_gen_rt_2<realtime>, index, free_buf, endpoint));
}

template <bool realtime>
void b_gen_rt_2(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    sc_factory->buffer_sync(index);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_gen_nrt_3, index, free_buf, endpoint));
}

const char * b_free = "/b_free";
void b_gen_nrt_3(uint32_t index, sample * free_buf, nova_endpoint const & endpoint)
{
    free_aligned(free_buf);
    send_done_message(endpoint, b_free, index);
}

template <bool realtime>
void handle_b_gen(received_message const & msg, size_t msg_size, nova_endpoint const & endpoint)
{
    movable_array<char> cmd (msg_size, msg.TypeTags()-1);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(b_gen_nrt_1<realtime>, cmd, endpoint));
}


void handle_c_set(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();

    while (it != msg.ArgumentsEnd()) {
        osc::int32 bus_index = it->AsInt32(); ++it;
        float value = extract_float_argument(it++);

        sc_factory->controlbus_set(bus_index, value);
    }
}

void handle_c_setn(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();

    while (it != msg.ArgumentsEnd()) {
        osc::int32 bus_index, bus_count;
        bus_index = it->AsInt32(); ++it;
        bus_count = it->AsInt32(); ++it;

        for (int i = 0; i != bus_count; ++i) {
            float value = extract_float_argument(it++);
            sc_factory->controlbus_set(bus_index + i, value);
        }
    }
}

void handle_c_fill(received_message const & msg)
{
    osc::ReceivedMessageArgumentIterator it = msg.ArgumentsBegin();

    while (it != msg.ArgumentsEnd()) {
        osc::int32 bus_index, bus_count;
        bus_index = it->AsInt32(); ++it;
        bus_count = it->AsInt32(); ++it;
        float value = extract_float_argument(it++);
        sc_factory->controlbus_fill(bus_index, bus_count, value);
    }
}

template <bool realtime>
void handle_c_get(received_message const & msg,
                  nova_endpoint const & endpoint)
{
    const size_t elem_size = sizeof(int) + sizeof(float);
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    const size_t index_count = msg.ArgumentCount();
    const size_t alloc_size = index_count * elem_size + 128; /* hopefully enough */

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/c_set");

    while (!args.Eos())
    {
        osc::int32 index;
        args >> index;

        p << index << sc_factory->controlbus_get(index);
    }

    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}

template <bool realtime>
void handle_c_getn(received_message const & msg, nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    /* we pessimize, but better to allocate too much than too little */
    const size_t alloc_size = 128 +
        (2 * sizeof(int) + 128*sizeof(float)) * msg.ArgumentCount();

    sized_array<char, rt_pool_allocator<char> > return_message(alloc_size);

    osc::OutboundPacketStream p(return_message.c_array(), alloc_size);
    p << osc::BeginMessage("/c_setn");

    while (!args.Eos())
    {
        osc::int32 bus_index, bus_count;
        args >> bus_index >> bus_count;
        p << bus_index << bus_count;

        for (int i = 0; i != bus_count; ++i) {
            float value = sc_factory->controlbus_get(bus_index + i);
            p << value;
        }
    }

    p << osc::EndMessage;

    movable_array<char> message(p.Size(), return_message.c_array());
    cmd_dispatcher<realtime>::fire_io_callback(boost::bind(send_udp_message, message, endpoint));
}

std::pair<sc_synth_prototype_ptr *, size_t> wrap_synthdefs(std::vector<sc_synthdef> const & defs)
{
    size_t count = defs.size();
    sc_synth_prototype_ptr * prototypes = new sc_synth_prototype_ptr [count];

    for (size_t i = 0; i != count; ++i)
        prototypes[i].reset(new sc_synth_prototype(defs[i]));
    return std::make_pair(prototypes, count);
}

template <bool realtime>
void d_recv_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                nova_endpoint const & endpoint);
void d_recv_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint);

template <bool realtime>
void d_recv_nrt(movable_array<char> & def, completion_message & msg, nova_endpoint const & endpoint)
{
    size_t count;
    sc_synth_prototype_ptr * prototypes;
    boost::tie(prototypes, count) = wrap_synthdefs(read_synthdefs(def.data()));

    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(d_recv_rt2<realtime>, prototypes, count, msg, endpoint));
}

template <bool realtime>
void d_recv_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                nova_endpoint const & endpoint)
{
    std::for_each(prototypes, prototypes + prototype_count,
                  boost::bind(&synth_factory::register_prototype, instance, _1));

    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_recv_nrt3, prototypes, endpoint));
}

void d_recv_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint)
{
    delete[] prototypes;
    send_done_message(endpoint, "/d_recv");
}

template <bool realtime>
void handle_d_recv(received_message const & msg,
                   nova_endpoint const & endpoint)
{
    const void * synthdef_data;
    unsigned long synthdef_size;

    osc::ReceivedMessageArgumentIterator args = msg.ArgumentsBegin();

    args->AsBlob(synthdef_data, synthdef_size); ++args;
    movable_array<char> def(synthdef_size, (const char*)synthdef_data);
    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_recv_nrt<realtime>, def, message, endpoint));
}

template <bool realtime>
void d_load_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                nova_endpoint const & endpoint);
void d_load_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint);

template <bool realtime>
void d_load_nrt(movable_string & path, completion_message & msg, nova_endpoint const & endpoint)
{
    size_t count;
    sc_synth_prototype_ptr * prototypes;
    /* todo: we need to implment some file name pattern matching */
    boost::tie(prototypes, count) = wrap_synthdefs(sc_read_synthdefs_file(path.c_str()));

    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(d_load_rt2<realtime>, prototypes, count, msg, endpoint));
}

template <bool realtime>
void d_load_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                nova_endpoint const & endpoint)
{
    std::for_each(prototypes, prototypes + prototype_count,
                  boost::bind(&synth_factory::register_prototype, instance, _1));

    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_load_nrt3, prototypes, endpoint));
}

void d_load_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint)
{
    delete[] prototypes;
    send_done_message(endpoint, "/d_load");
}


template <bool realtime>
void handle_d_load(received_message const & msg,
                   nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentIterator args = msg.ArgumentsBegin();
    const char * path = args->AsString(); args++;
    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_load_nrt<realtime>, movable_string(path),
                                                               message, endpoint));
}


template <bool realtime>
void d_loadDir_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                   nova_endpoint const & endpoint);
void d_loadDir_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint);

template <bool realtime>
void d_loadDir_nrt1(movable_string & path, completion_message & msg, nova_endpoint const & endpoint)
{
    size_t count;
    sc_synth_prototype_ptr * prototypes;
    boost::tie(prototypes, count) = wrap_synthdefs(sc_read_synthdefs_dir(path.c_str()));

    cmd_dispatcher<realtime>::fire_rt_callback(boost::bind(d_loadDir_rt2<realtime>, prototypes, count, msg, endpoint));
}

template <bool realtime>
void d_loadDir_rt2(sc_synth_prototype_ptr * prototypes, size_t prototype_count, completion_message & msg,
                   nova_endpoint const & endpoint)
{
    std::for_each(prototypes, prototypes + prototype_count,
                  boost::bind(&synth_factory::register_prototype, instance, _1));

    msg.handle(endpoint);
    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_loadDir_nrt3, prototypes, endpoint));
}

void d_loadDir_nrt3(sc_synth_prototype_ptr * prototypes, nova_endpoint const & endpoint)
{
    delete[] prototypes;
    send_done_message(endpoint, "/d_loadDir");
}

template <bool realtime>
void handle_d_loadDir(received_message const & msg,
                      nova_endpoint const & endpoint)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    const char * path;

    args >> path;
    completion_message message = extract_completion_message(args);

    cmd_dispatcher<realtime>::fire_system_callback(boost::bind(d_loadDir_nrt1<realtime>,
                                                               movable_string(path), message, endpoint));
}


void handle_d_free(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        const char * defname;
        args >> defname;

        instance->remove_prototype(defname);
    }
}

void insert_parallel_group(int node_id, int action, int target_id)
{
    if (node_id == -1)
        node_id = instance->generate_node_id();
    else if (!check_node_id(node_id))
        return;

    server_node * target = find_node(target_id);
    if(!target)
        return;

    node_position_constraint pos = make_pair(target, node_position(action));

    instance->add_parallel_group(node_id, pos);
    last_generated = node_id;
}

void handle_p_new(received_message const & msg)
{
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();

    while(!args.Eos())
    {
        osc::int32 id, action, target;
        args >> id >> action >> target;

        insert_parallel_group(id, action, target);
    }
}

void handle_u_cmd(received_message const & msg, int size)
{
    sc_msg_iter args(size, msg.AddressPattern());

    int node_id = args.geti();

    server_node * target_synth = find_node(node_id);

    if (target_synth == NULL || target_synth->is_group())
        return;

    sc_synth * synth = static_cast<sc_synth*>(target_synth);

    int ugen_index = args.geti();
    const char * cmd_name = args.gets();

    synth->apply_unit_cmd(cmd_name, ugen_index, &args);
}

} /* namespace */

template <bool realtime>
void sc_osc_handler::handle_message_int_address(received_message const & message,
                                                size_t msg_size, nova_endpoint const & endpoint)
{
    uint32_t address = message.AddressPatternAsUInt32();

    switch (address)
    {
    case cmd_quit:
        handle_quit<realtime>(endpoint);
        break;

    case cmd_s_new:
        handle_s_new(message);
        break;

    case cmd_s_noid:
        handle_s_noid(message);
        break;

    case cmd_s_get:
        handle_s_get<realtime>(message, msg_size, endpoint);
        break;

    case cmd_s_getn:
        handle_s_getn<realtime>(message, msg_size, endpoint);
        break;

    case cmd_notify:
        handle_notify<realtime>(message, endpoint);
        break;

    case cmd_status:
        handle_status<realtime>(endpoint);
        break;

    case cmd_dumpOSC:
        handle_dumpOSC(message);
        break;

    case cmd_sync:
        handle_sync<realtime>(message, endpoint);
        break;

    case cmd_clearSched:
        handle_clearSched();
        break;

    case cmd_error:
        handle_error(message);
        break;

    case cmd_g_new:
        handle_g_new(message);
        break;

    case cmd_g_head:
        handle_g_head(message);
        break;

    case cmd_g_tail:
        handle_g_tail(message);
        break;

    case cmd_g_freeAll:
        handle_g_freeall(message);
        break;

    case cmd_g_deepFree:
        handle_g_deepFree(message);
        break;

    case cmd_g_queryTree:
        handle_g_queryTree<realtime>(message, endpoint);
        break;

    case cmd_g_dumpTree:
        handle_g_dumpTree(message);
        break;

    case cmd_n_free:
        handle_n_free(message);
        break;

    case cmd_n_set:
        handle_n_set(message);
        break;

    case cmd_n_setn:
        handle_n_setn(message);
        break;

    case cmd_n_fill:
        handle_n_fill(message);
        break;

    case cmd_n_map:
        handle_n_map(message);
        break;

    case cmd_n_mapn:
        handle_n_mapn(message);
        break;

    case cmd_n_mapa:
        handle_n_mapa(message);
        break;

    case cmd_n_mapan:
        handle_n_mapan(message);
        break;

    case cmd_n_query:
        handle_n_query(message, endpoint);
        break;

    case cmd_n_order:
        handle_n_order(message);
        break;

    case cmd_n_run:
        handle_n_run(message);
        break;

    case cmd_n_before:
        handle_n_before(message);
        break;

    case cmd_n_after:
        handle_n_after(message);
        break;

    case cmd_n_trace:
        handle_n_trace(message);
        break;

    case cmd_b_alloc:
        handle_b_alloc<realtime>(message, endpoint);
        break;

    case cmd_u_cmd:
        handle_u_cmd(message, msg_size);
        break;

    case cmd_b_free:
        handle_b_free<realtime>(message, endpoint);
        break;

    case cmd_b_allocRead:
        handle_b_allocRead<realtime>(message, endpoint);
        break;

    case cmd_b_allocReadChannel:
        handle_b_allocReadChannel<realtime>(message, endpoint);
        break;

    case cmd_b_read:
        handle_b_read<realtime>(message, endpoint);
        break;

    case cmd_b_readChannel:
        handle_b_readChannel<realtime>(message, endpoint);
        break;

    case cmd_b_write:
        handle_b_write<realtime>(message, endpoint);
        break;

    case cmd_b_zero:
        handle_b_zero<realtime>(message, endpoint);
        break;

    case cmd_b_set:
        handle_b_set(message);
        break;

    case cmd_b_setn:
        handle_b_setn(message);
        break;

    case cmd_b_fill:
        handle_b_fill(message);
        break;

    case cmd_b_query:
        handle_b_query<realtime>(message, endpoint);
        break;

    case cmd_b_get:
        handle_b_get<realtime>(message, endpoint);
        break;

    case cmd_b_getn:
        handle_b_getn<realtime>(message, endpoint);
        break;

    case cmd_b_gen:
        handle_b_gen<realtime>(message, msg_size, endpoint);
        break;

    case cmd_c_set:
        handle_c_set(message);
        break;

    case cmd_c_setn:
        handle_c_setn(message);
        break;

    case cmd_c_fill:
        handle_c_fill(message);
        break;

    case cmd_c_get:
        handle_c_get<realtime>(message, endpoint);
        break;

    case cmd_c_getn:
        handle_c_getn<realtime>(message, endpoint);
        break;

    case cmd_d_recv:
        handle_d_recv<realtime>(message, endpoint);
        break;

    case cmd_d_load:
        handle_d_load<realtime>(message, endpoint);
        break;

    case cmd_d_loadDir:
        handle_d_loadDir<realtime>(message, endpoint);
        break;

    case cmd_d_free:
        handle_d_free(message);
        break;

    case cmd_p_new:
        handle_p_new(message);
        break;

    default:
        handle_unhandled_message(message);
    }
}

namespace
{

template <bool realtime>
void dispatch_group_commands(const char * address, received_message const & message,
                             nova_endpoint const & endpoint)
{
    assert(address[1] == 'g');
    assert(address[2] == '_');

    if (strcmp(address+3, "new") == 0) {
        handle_g_new(message);
        return;
    }
    if (strcmp(address+3, "head") == 0) {
        handle_g_head(message);
        return;
    }
    if (strcmp(address+3, "tail") == 0) {
        handle_g_tail(message);
        return;
    }
    if (strcmp(address+3, "freeAll") == 0) {
        handle_g_freeall(message);
        return;
    }
    if (strcmp(address+3, "deepFree") == 0) {
        handle_g_deepFree(message);
        return;
    }
    if (strcmp(address+3, "queryTree") == 0) {
        handle_g_queryTree<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "dumpTree") == 0) {
        handle_g_dumpTree(message);
        return;
    }
}

template <bool realtime>
void dispatch_node_commands(const char * address, received_message const & message,
                            nova_endpoint const & endpoint)
{
    assert(address[1] == 'n');
    assert(address[2] == '_');

    if (strcmp(address+3, "free") == 0) {
        handle_n_free(message);
        return;
    }

    if (strcmp(address+3, "set") == 0) {
        handle_n_set(message);
        return;
    }

    if (strcmp(address+3, "setn") == 0) {
        handle_n_setn(message);
        return;
    }

    if (strcmp(address+3, "fill") == 0) {
        handle_n_fill(message);
        return;
    }

    if (strcmp(address+3, "map") == 0) {
        handle_n_map(message);
        return;
    }

    if (strcmp(address+3, "mapn") == 0) {
        handle_n_mapn(message);
        return;
    }

    if (strcmp(address+3, "mapa") == 0) {
        handle_n_mapa(message);
        return;
    }

    if (strcmp(address+3, "mapan") == 0) {
        handle_n_mapan(message);
        return;
    }

    if (strcmp(address+3, "run") == 0) {
        handle_n_run(message);
        return;
    }

    if (strcmp(address+3, "before") == 0) {
        handle_n_before(message);
        return;
    }

    if (strcmp(address+3, "after") == 0) {
        handle_n_after(message);
        return;
    }

    if (strcmp(address+3, "order") == 0) {
        handle_n_order(message);
        return;
    }

    if (strcmp(address+3, "query") == 0) {
        handle_n_query(message, endpoint);
        return;
    }

    if (strcmp(address+3, "trace") == 0) {
        handle_n_trace(message);
        return;
    }
}

template <bool realtime>
void dispatch_buffer_commands(const char * address, received_message const & message,
                              size_t msg_size, nova_endpoint const & endpoint)
{
    assert(address[1] == 'b');
    assert(address[2] == '_');

    if (strcmp(address+3, "alloc") == 0) {
        handle_b_alloc<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "free") == 0) {
        handle_b_free<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "allocRead") == 0) {
        handle_b_allocRead<realtime>(message, endpoint);
        return;
    }
    if (strcmp(address+3, "allocReadChannel") == 0) {
        handle_b_allocReadChannel<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "read") == 0) {
        handle_b_read<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "readChannel") == 0) {
        handle_b_readChannel<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "write") == 0) {
        handle_b_write<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "zero") == 0) {
        handle_b_zero<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "set") == 0) {
        handle_b_set(message);
        return;
    }

    if (strcmp(address+3, "setn") == 0) {
        handle_b_setn(message);
        return;
    }

    if (strcmp(address+3, "fill") == 0) {
        handle_b_fill(message);
        return;
    }

    if (strcmp(address+3, "query") == 0) {
        handle_b_query<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "get") == 0) {
        handle_b_get<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "getn") == 0) {
        handle_b_getn<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "gen") == 0) {
        handle_b_gen<realtime>(message, msg_size, endpoint);
        return;
    }
}

template <bool realtime>
void dispatch_control_bus_commands(const char * address, received_message const & message,
                                   nova_endpoint const & endpoint)
{
    assert(address[1] == 'c');
    assert(address[2] == '_');

    if (strcmp(address+3, "set") == 0) {
        handle_c_set(message);
        return;
    }

    if (strcmp(address+3, "setn") == 0) {
        handle_c_setn(message);
        return;
    }

    if (strcmp(address+3, "fill") == 0) {
        handle_c_fill(message);
        return;
    }

    if (strcmp(address+3, "get") == 0) {
        handle_c_get<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "getn") == 0) {
        handle_c_getn<realtime>(message, endpoint);
        return;
    }
}

template <bool realtime>
void dispatch_synthdef_commands(const char * address, received_message const & message,
                                nova_endpoint const & endpoint)
{
    assert(address[1] == 'd');
    assert(address[2] == '_');

    if (strcmp(address+3, "recv") == 0) {
        handle_d_recv<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "load") == 0) {
        handle_d_load<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "loadDir") == 0) {
        handle_d_loadDir<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+3, "free") == 0) {
        handle_d_free(message);
        return;
    }
}

template <bool realtime>
void dispatch_synth_commands(const char * address, received_message const & message, size_t msg_size,
                             nova_endpoint const & endpoint)
{
    assert(address[1] == 's');
    assert(address[2] == '_');

    if (strcmp(address+3, "new") == 0) {
        handle_s_new(message);
        return;
    }

    if (strcmp(address+3, "noid") == 0) {
        handle_s_noid(message);
        return;
    }

    if (strcmp(address+3, "get") == 0) {
        handle_s_get<realtime>(message, msg_size, endpoint);
        return;
    }

    if (strcmp(address+3, "getn") == 0) {
        handle_s_getn<realtime>(message, msg_size, endpoint);
        return;
    }
}

} /* namespace */

template <bool realtime>
void sc_osc_handler::handle_message_sym_address(received_message const & message,
                                                size_t msg_size, nova_endpoint const & endpoint)
{
    const char * address = message.AddressPattern();

    /* scsynth doesn't require the leading / */
    if(address[0] != '/')
        address -= 1;

    if (address[2] == '_')
    {
        if (address[1] == 'g') {
            dispatch_group_commands<realtime>(address, message, endpoint);
            return;
        }

        if (address[1] == 'n') {
            dispatch_node_commands<realtime>(address, message, endpoint);
            return;
        }

        if (address[1] == 'b') {
            dispatch_buffer_commands<realtime>(address, message, msg_size, endpoint);
            return;
        }

        if (address[1] == 'c') {
            dispatch_control_bus_commands<realtime>(address, message, endpoint);
            return;
        }

        if (address[1] == 'd') {
            dispatch_synthdef_commands<realtime>(address, message, endpoint);
            return;
        }

        if (address[1] == 's') {
            dispatch_synth_commands<realtime>(address, message, msg_size, endpoint);
            return;
        }
    }

    if (strcmp(address+1, "p_new") == 0) {
        handle_p_new(message);
        return;
    }

    if (strcmp(address+1, "u_cmd") == 0) {
        handle_u_cmd(message, msg_size);
        return;
    }

    if (strcmp(address+1, "status") == 0) {
        handle_status<realtime>(endpoint);
        return;
    }

    if (strcmp(address+1, "sync") == 0) {
        handle_sync<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+1, "quit") == 0) {
        handle_quit<realtime>(endpoint);
        return;
    }

    if (strcmp(address+1, "notify") == 0) {
        handle_notify<realtime>(message, endpoint);
        return;
    }

    if (strcmp(address+1, "dumpOSC") == 0) {
        handle_dumpOSC(message);
        return;
    }

    if (strcmp(address+1, "clearSched") == 0) {
        handle_clearSched();
        return;
    }

    if (strcmp(address+1, "error") == 0) {
        handle_error(message);
        return;
    }

    handle_unhandled_message(message);
}

} /* namespace detail */
} /* namespace nova */
