/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_TCP_AUTO_CLIENT_IMPL_HPP__
#define __ASIO2_TCP_AUTO_CLIENT_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio2/tcp/tcp_client_impl.hpp>
#include <asio2/tcp/tcp_auto_connection_impl.hpp>

namespace asio2
{

	template<class _connection_impl_t>
	class tcp_auto_client_impl : public tcp_client_impl<_connection_impl_t>
	{
	public:

		/**
		 * @construct
		 */
		explicit tcp_auto_client_impl(
			std::shared_ptr<url_parser>                    url_parser_ptr,
			std::shared_ptr<listener_mgr>                  listener_mgr_ptr
		)
			: tcp_client_impl<_connection_impl_t>(url_parser_ptr, listener_mgr_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~tcp_auto_client_impl()
		{
		}


	};

}

#endif // !__ASIO2_TCP_AUTO_CLIENT_IMPL_HPP__
