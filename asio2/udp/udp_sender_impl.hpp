/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_UDP_SENDER_IMPL_HPP__
#define __ASIO2_UDP_SENDER_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio2/base/sender_impl.hpp>

namespace asio2
{

	class udp_sender_impl : public sender_impl
	{
	public:
		/**
		 * @construct
		 */
		explicit udp_sender_impl(
			std::shared_ptr<url_parser>              url_parser_ptr,
			std::shared_ptr<listener_mgr>            listener_mgr_ptr
		)
			: sender_impl(url_parser_ptr, listener_mgr_ptr)
			, m_socket(*m_recv_io_context_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~udp_sender_impl()
		{
		}

		/**
		 * @function : start the sender.you must call the stop function before application exit,otherwise will cause crash.
		 * @return   : true  - start successed 
		 *             false - start failed
		 */
		virtual bool start() override
		{
			try
			{
				// first call base class start function
				if (!sender_impl::start())
					return false;

				m_state = state::starting;

				// reset the state to the default
				m_fire_close_is_called.clear(std::memory_order_release);

				// parse address and port
				boost::asio::ip::udp::resolver resolver(*m_recv_io_context_ptr);
				boost::asio::ip::udp::resolver::query query(m_url_parser_ptr->get_ip(), m_url_parser_ptr->get_port());
				boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

				m_socket.open(endpoint.protocol());
				m_socket.bind(endpoint);

				// setsockopt SO_SNDBUF from url params
				if (m_url_parser_ptr->get_so_sndbuf_size() > 0)
				{
					boost::asio::socket_base::send_buffer_size option(m_url_parser_ptr->get_so_sndbuf_size());
					m_socket.set_option(option);
				}

				// setsockopt SO_RCVBUF from url params
				if (m_url_parser_ptr->get_so_rcvbuf_size() > 0)
				{
					boost::asio::socket_base::receive_buffer_size option(m_url_parser_ptr->get_so_rcvbuf_size());
					m_socket.set_option(option);
				}

				_post_recv(shared_from_this(), std::make_shared<buffer<uint8_t>>(
					m_url_parser_ptr->get_recv_buffer_size(), malloc_recv_buffer(m_url_parser_ptr->get_recv_buffer_size()), 0));

				m_state = state::running;

				return (m_socket.is_open());
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}

			return false;
		}

		/**
		 * @function : stop the sender
		 */
		virtual void stop() override
		{
			if (m_state >= state::starting)
			{
				m_state = state::stopping;

				// call listen socket's close function to notify the _handle_accept function response with error > 0 ,then the listen socket 
				// can get notify to exit
				if (m_socket.is_open())
				{
					try
					{
						auto self(shared_from_this());

						// first wait for all send pending complete
						auto promise_ptr = std::make_shared<std::promise<void>>();
						m_send_strand_ptr->post([this, self, promise_ptr]()
						{
							promise_ptr->set_value();
						});

						// asio don't allow operate the same socket in multi thread,if you close socket in one thread and another thread is 
						// calling socket's async_read... function,it will crash.so we must care for operate the socket.when need close the
						// socket ,we use the strand to post a event,make sure the socket's close operation is in the same thread.
						m_recv_strand_ptr->post([this, self, promise_ptr]()
						{
							// wait util the send event is finished compelted
							promise_ptr->get_future().wait();

							// close the socket
							try
							{
								if (m_state == state::running)
									_fire_close(get_last_error());

								m_socket.shutdown(boost::asio::socket_base::shutdown_both);
								m_socket.close();
							}
							catch (boost::system::system_error & e)
							{
								set_last_error(e.code().value());
							}

							m_state = state::stopped;
						});
					}
					catch (std::exception &) {}
				}

				// last call base class stop funtcion
				sender_impl::stop();
			}
		}

		/**
		 * @function : whether the sender is started
		 */
		virtual bool is_start() override
		{
			return ((m_state >= state::started) && m_socket.is_open());
		}

		/**
		 * @function : send data
		 */
		virtual bool send(std::string & ip, std::string & port, std::shared_ptr<buffer<uint8_t>> buf_ptr) override
		{
			try
			{
				if (is_start() && !ip.empty() && buf_ptr)
				{
					boost::asio::ip::udp::resolver resolver(*m_recv_io_context_ptr);
					boost::asio::ip::udp::resolver::query query(ip, port);
					boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

					// must use strand.post to send data.why we should do it like this ? see udp_session._post_send.
					m_send_strand_ptr->post(std::bind(&udp_sender_impl::_post_send, this,
						shared_from_this(),
						endpoint,
						buf_ptr
					));
					return true;
				}
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}
			return false;
		}

	public:
		/**
		 * @function : get the socket shared_ptr
		 */
		inline boost::asio::ip::udp::socket & get_socket()
		{
			return m_socket;
		}

		/**
		 * @function : get the local address
		 */
		virtual std::string get_local_address() override
		{
			try
			{
				if (m_socket.is_open())
				{
					return m_socket.local_endpoint().address().to_string();
				}
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the local port
		 */
		virtual unsigned short get_local_port() override
		{
			try
			{
				if (m_socket.is_open())
				{
					return m_socket.local_endpoint().port();
				}
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

		/**
		 * @function : get the remote address
		 */
		virtual std::string get_remote_address() override
		{
			try
			{
				if (m_socket.is_open())
				{
					return m_socket.remote_endpoint().address().to_string();
				}
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the remote port
		 */
		virtual unsigned short get_remote_port() override
		{
			try
			{
				if (m_socket.is_open())
				{
					return m_socket.remote_endpoint().port();
				}
			}
			catch (boost::system::system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

	protected:
		virtual void _post_recv(std::shared_ptr<sender_impl> this_ptr, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (is_start())
			{
				if (buf_ptr->remain() > 0)
				{
					const auto & buffer = boost::asio::buffer(buf_ptr->write_begin(), buf_ptr->remain());
					this->m_socket.async_receive_from(buffer, m_sender_endpoint,
						this->m_recv_strand_ptr->wrap(std::bind(&udp_sender_impl::_handle_recv, this,
							std::placeholders::_1, // error_code
							std::placeholders::_2, // bytes_recvd
							std::move(this_ptr),
							std::move(buf_ptr)
						)));
				}
				else
				{
					set_last_error((int)errcode::recv_buffer_size_too_small);
					PRINT_EXCEPTION;
					this->stop();
					assert(false);
				}
			}
		}

		virtual void _handle_recv(const boost::system::error_code & ec, std::size_t bytes_recvd, std::shared_ptr<sender_impl> this_ptr, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (!ec)
			{
				if (bytes_recvd == 0)
				{
					// recvd data len is 0,may be heartbeat packet.
				}
				else if (bytes_recvd > 0)
				{
					buf_ptr->write_bytes(bytes_recvd);
				}

				auto use_count = buf_ptr.use_count();

				_fire_recv(m_sender_endpoint, buf_ptr);

				if (use_count == buf_ptr.use_count())
				{
					buf_ptr->reset();
					this->_post_recv(std::move(this_ptr), std::move(buf_ptr));
				}
				else
				{
					this->_post_recv(std::move(this_ptr), std::make_shared<buffer<uint8_t>>(
						m_url_parser_ptr->get_recv_buffer_size(), malloc_recv_buffer(m_url_parser_ptr->get_recv_buffer_size()), 0));
				}
			}
			else
			{
				set_last_error(ec.value());

				// close this sender
				this->stop();
			}
		}

		virtual void _post_send(std::shared_ptr<sender_impl> this_ptr, boost::asio::ip::udp::endpoint endpoint, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (is_start())
			{
				boost::system::error_code ec;
				m_socket.send_to(boost::asio::buffer(buf_ptr->read_begin(), buf_ptr->size()), endpoint, 0, ec);
				set_last_error(ec.value());
				this->_fire_send(endpoint, buf_ptr, ec.value());
				if (ec)
				{
					PRINT_EXCEPTION;
					this->stop();
				}
			}
			else
			{
				set_last_error((int)errcode::socket_not_ready);
				this->_fire_send(endpoint, buf_ptr, get_last_error());
			}
		}

		virtual void _fire_recv(boost::asio::ip::udp::endpoint & endpoint, std::shared_ptr<buffer<uint8_t>> & buf_ptr)
		{
			auto ip = endpoint.address().to_string();
			static_cast<sender_listener_mgr *>(m_listener_mgr_ptr.get())->notify_recv(ip, endpoint.port(), buf_ptr);
		}

		virtual void _fire_send(boost::asio::ip::udp::endpoint & endpoint, std::shared_ptr<buffer<uint8_t>> & buf_ptr, int error)
		{
			auto ip = endpoint.address().to_string();
			static_cast<sender_listener_mgr *>(m_listener_mgr_ptr.get())->notify_send(ip, endpoint.port(), buf_ptr, error);
		}

		virtual void _fire_close(int error)
		{
			if (!m_fire_close_is_called.test_and_set(std::memory_order_acquire))
			{
				dynamic_cast<sender_listener_mgr *>(m_listener_mgr_ptr.get())->notify_close(error);
			}
		}

	protected:
		/// socket
		boost::asio::ip::udp::socket m_socket;

		/// use to avoid call _fire_close twice
		std::atomic_flag m_fire_close_is_called = ATOMIC_FLAG_INIT;

		/// endpoint for udp 
		boost::asio::ip::udp::endpoint m_sender_endpoint;

	};
}

#endif // !__ASIO2_UDP_SENDER_IMPL_HPP__
