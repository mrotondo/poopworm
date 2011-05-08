//  osc handler for supercollider-style communication
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

#ifndef SERVER_SC_OSC_HANDLER_HPP
#define SERVER_SC_OSC_HANDLER_HPP

#include <vector>
#include <algorithm>

#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/intrusive/treap_set.hpp>

#include "osc/OscReceivedElements.h"

#include "../server/dynamic_endpoint.hpp"
#include "../server/memory_pool.hpp"
#include "../server/server_args.hpp"
#include "../server/server_scheduler.hpp"
#include "../utilities/osc_server.hpp"
#include "../utilities/sized_array.hpp"
#include "../utilities/static_pool.hpp"
#include "../utilities/time_tag.hpp"


namespace nova
{

namespace detail
{
using namespace boost::asio;
using namespace boost::asio::ip;

/**
 * observer to receive osc notifications
 *
 * \todo shall we use a separate thread for observer notifications?
 * */
class sc_notify_observers
{
    typedef std::vector<nova_endpoint> observer_vector;

public:
    sc_notify_observers(boost::asio::io_service & io_service):
        udp_socket(io_service), tcp_socket(io_service)
    {}

    void add_observer(nova_endpoint const & ep)
    {
        observers.push_back(ep);
    }

    void remove_observer(nova_endpoint const & ep)
    {
        observer_vector::iterator it = std::find(observers.begin(),
                                                 observers.end(), ep);
        assert (it != observers.end());

        observers.erase(it);
    }


    /* @{ */
    /** notifications, should be called from the real-time thread */
    void notification_node_started(const server_node * node)
    {
        notify("/n_go", node);
    }

    void notification_node_ended(const server_node * node)
    {
        notify("/n_end", node);
    }

    void notification_node_turned_off(const server_node * node)
    {
        notify("/n_off", node);
    }

    void notification_node_turned_on(const server_node * node)
    {
        notify("/n_on", node);
    }

    void notification_node_moved(const server_node * node)
    {
        notify("/n_move", node);
    }

    void send_trigger(int32_t node_id, int32_t trigger_id, float value);

    void send_node_reply(int32_t node_id, int reply_id, const char* command_name, int argument_count, const float* values);
    /* @} */

    /** send notifications, should not be called from the real-time thread */
    void send_notification(const char * data, size_t length);

    /* @{ */
    /** sending functions */
    void send(const char * data, size_t size, nova_endpoint const & endpoint)
    {
        nova_protocol prot = endpoint.protocol();
        if (prot.family() == AF_INET && prot.type() == SOCK_DGRAM)
        {
            udp::endpoint ep(endpoint.address(), endpoint.port());
            send_udp(data, size, ep);
        }
        else if (prot.family() == AF_INET && prot.type() == SOCK_STREAM)
        {
            tcp::endpoint ep(endpoint.address(), endpoint.port());
            send_tcp(data, size, ep);
        }
    }

    void send_udp(const char * data, unsigned int size, udp::endpoint const & receiver)
    {
        boost::mutex::scoped_lock lock(udp_mutex);
        sc_notify_observers::udp_socket.send_to(boost::asio::buffer(data, size), receiver);
    }

    void send_tcp(const char * data, unsigned int size, tcp::endpoint const & receiver)
    {
        boost::mutex::scoped_lock lock(tcp_mutex);
        tcp_socket.connect(receiver);
        boost::asio::write(tcp_socket, boost::asio::buffer(data, size));
    }
    /* @} */

private:
    void notify(const char * address_pattern, const server_node * node);
    void send_notification(const char * data, size_t length, nova_endpoint const & endpoint);

    observer_vector observers;

protected:
    udp::socket udp_socket;
    tcp::socket tcp_socket;
    boost::mutex udp_mutex, tcp_mutex;
};

class sc_scheduled_bundles
{
public:
    struct bundle_node:
        public boost::intrusive::bs_set_base_hook<>
    {
        bundle_node(time_tag const & timeout, const char * data, nova_endpoint const & endpoint):
            timeout_(timeout), data_(data), endpoint_(endpoint)
        {}

        void run(void);

        const time_tag timeout_;
        const char * const data_;
        const nova_endpoint endpoint_;

        friend bool operator< (const bundle_node & lhs, const bundle_node & rhs)
        {
            return priority_order(lhs, rhs);
        }

        friend bool priority_order (const bundle_node & lhs, const bundle_node & rhs)
        {
            return lhs.timeout_ < rhs.timeout_; // lower value, higher priority
        }
    };

    typedef boost::intrusive::treap_multiset<bundle_node> bundle_queue_t;

    void insert_bundle(time_tag const & timeout, const char * data, size_t length,
                       nova_endpoint const & endpoint);

    void execute_bundles(time_tag const & now);

    void clear_bundles(void)
    {
        bundle_q.clear_and_dispose(dispose_bundle);
    }

    static void dispose_bundle(bundle_node * node)
    {
        node->~bundle_node();
        rt_pool.free(node);
    }

private:
    bundle_queue_t bundle_q;
};

class sc_osc_handler:
    private detail::network_thread,
    public sc_notify_observers
{
    /* @{ */
    /** constructor helpers */
    void open_tcp_acceptor(tcp const & protocol, unsigned int port);
    void open_udp_socket(udp const & protocol, unsigned int port);
    bool open_socket(int family, int type, int protocol, unsigned int port);
    /* @} */

public:
    sc_osc_handler(server_arguments const & args):
        sc_notify_observers(detail::network_thread::io_service_),
        dump_osc_packets(0), error_posting(1),
        tcp_acceptor_(detail::network_thread::io_service_),
        tcp_password_(args.server_password.c_str())
    {
        if (args.tcp_port && !open_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, args.tcp_port))
            throw std::runtime_error("cannot open socket");
        if (args.udp_port && !open_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, args.udp_port))
            throw std::runtime_error("cannot open socket");
    }

    ~sc_osc_handler(void)
    {}

    typedef osc::ReceivedPacket osc_received_packet;
    typedef osc::ReceivedBundle received_bundle;
    typedef osc::ReceivedMessage received_message;

    struct received_packet:
        public audio_sync_callback
    {
        received_packet(const char * dat, size_t length, nova_endpoint const & endpoint):
            data(dat), length(length), endpoint_(endpoint)
        {}

        void * operator new(std::size_t size, void* ptr)
        {
            return ::operator new(size, ptr);
        }

        static received_packet * alloc_packet(const char * data, size_t length,
                                              nova_endpoint const & remote_endpoint);

        void run(void);

        const char * const data;
        const size_t length;
        const nova_endpoint endpoint_;
    };

private:
    /* @{ */
    /** udp socket handling */
    void start_receive_udp(void)
    {
        sc_notify_observers::udp_socket.async_receive_from(
            buffer(recv_buffer_), udp_remote_endpoint_,
            boost::bind(&sc_osc_handler::handle_receive_udp, this,
                        placeholders::error, placeholders::bytes_transferred));
    }

    void handle_receive_udp(const boost::system::error_code& error,
                            std::size_t bytes_transferred)
    {
        if (unlikely(error == error::operation_aborted))
            return;    /* we're done */

        if (error == error::message_size)
        {
            overflow_vector.insert(overflow_vector.end(),
                                   recv_buffer_.begin(), recv_buffer_.end());
            return;
        }

        if (error)
        {
            std::cout << "sc_osc_handler received error code " << error << std::endl;
            start_receive_udp();
            return;
        }

        if (overflow_vector.empty())
            handle_packet_async(recv_buffer_.begin(), bytes_transferred, udp_remote_endpoint_);
        else
        {
            overflow_vector.insert(overflow_vector.end(),
                                   recv_buffer_.begin(), recv_buffer_.end());

            handle_packet_async(overflow_vector.data(), overflow_vector.size(), udp_remote_endpoint_);

            overflow_vector.clear();
        }

        start_receive_udp();
        return;
    }
    /* @} */

    /* @{ */
    /** tcp connection handling */
    class tcp_connection:
        public boost::enable_shared_from_this<tcp_connection>
    {
    public:
        typedef boost::shared_ptr<tcp_connection> pointer;

        static pointer create(boost::asio::io_service& io_service)
        {
            return pointer(new tcp_connection(io_service));
        }

        tcp::socket& socket()
        {
            return socket_;
        }

        void start(sc_osc_handler * self)
        {
            bool check_password = true;

            if (check_password)
            {
                boost::array<char, 32> password;
                size_t size;
                uint32_t msglen;
                for (unsigned int i=0; i!=4; ++i)
                {
                    size = socket_.receive(boost::asio::buffer(&msglen, 4));
                    if (size != 4)
                        return;

                    msglen = ntohl(msglen);
                    if (msglen > password.size())
                        return;

                    size = socket_.receive(boost::asio::buffer(password.data(), msglen));

                    bool verified = true;
                    if (size != msglen ||
                        strcmp(password.data(), self->tcp_password_) != 0)
                        verified = false;

                    if (!verified)
                        throw std::runtime_error("cannot verify password");
                }
            }

            size_t size;
            uint32_t msglen;
            size = socket_.receive(boost::asio::buffer(&msglen, 4));
            if (size != sizeof(uint32_t))
                throw std::runtime_error("read error");

            msglen = ntohl(msglen);

            sized_array<char> recv_vector(msglen + sizeof(uint32_t));

            std::memcpy((void*)recv_vector.data(), &msglen, sizeof(uint32_t));
            size_t transfered = socket_.read_some(boost::asio::buffer((void*)(recv_vector.data()+sizeof(uint32_t)),
                                                                      recv_vector.size()-sizeof(uint32_t)));

            if (transfered != size_t(msglen))
                throw std::runtime_error("socket read sanity check failure");

            self->handle_packet_async(recv_vector.data(), recv_vector.size(), socket_.remote_endpoint());
        }

    private:
        tcp_connection(boost::asio::io_service& io_service)
            : socket_(io_service)
        {}

        tcp::socket socket_;
    };

    void start_accept(void)
    {
        tcp_connection::pointer new_connection = tcp_connection::create(tcp_acceptor_.get_io_service());

        tcp_acceptor_.async_accept(new_connection->socket(),
            boost::bind(&sc_osc_handler::handle_accept, this, new_connection,
            boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start(this);
            start_accept();
        }
    }
    /* @} */

public:
    void dumpOSC(int i)
    {
        dump_osc_packets = i;
    }

private:
    int dump_osc_packets;

    /* @{ */
public:
    /** \todo how to handle temporary message error suppression? */

    void set_error_posting(int val)
    {
        error_posting = val;
    }

private:
    int error_posting;
    /* @} */

    /* @{ */
    /** packet handling */
public:
    void handle_packet_async(const char* data, size_t length, nova::nova_endpoint const & endpoint);
    void handle_packet(const char* data, size_t length, nova::nova_endpoint const & endpoint);
    time_tag handle_bundle_nrt(const char * data_, std::size_t length);

private:
    template <bool realtime>
    void handle_bundle(received_bundle const & bundle, nova_endpoint const & endpoint);
    template <bool realtime>
    void handle_message(received_message const & message, size_t msg_size, nova_endpoint const & endpoint);
    template <bool realtime>
    void handle_message_int_address(received_message const & message, size_t msg_size, nova_endpoint const & endpoint);
    template <bool realtime>
    void handle_message_sym_address(received_message const & message, size_t msg_size, nova_endpoint const & endpoint);

    friend class sc_scheduled_bundles::bundle_node;
    /* @} */

    /* @{ */
    /** bundle scheduling */
public:
    void clear_scheduled_bundles(void)
    {
        scheduled_bundles.clear_bundles();
    }

    void execute_scheduled_bundles(void)
    {
        scheduled_bundles.execute_bundles(now);
    }

    void increment_logical_time(time_tag const & diff)
    {
        now += diff;
    }

    void update_time_from_system(void)
    {
        now = time_tag::from_ptime(boost::date_time::microsec_clock<boost::posix_time::ptime>::universal_time());
    }

    time_tag const & current_time(void) const
    {
        return now;
    }

    sc_scheduled_bundles scheduled_bundles;
    time_tag now;
/* @} */

private:
    /* @{ */
/*    udp::socket udp_socket_;*/
    udp::endpoint udp_remote_endpoint_;

    tcp::acceptor tcp_acceptor_;
    const char * tcp_password_; /* we are not owning this! */

    boost::array<char, 1<<15 > recv_buffer_;

    std::vector<char> overflow_vector;
    /* @} */
};

} /* namespace detail */

using detail::sc_osc_handler;

} /* namespace nova */


#endif /* SERVER_SC_OSC_HANDLER_HPP */
