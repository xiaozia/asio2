/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_TCPS_AUTO_CLIENT_IMPL_HPP__
#define __ASIO2_TCPS_AUTO_CLIENT_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio2/tcp/tcps_auto_connection_impl.hpp>
#include <asio2/tcp/tcps_client_impl.hpp>

namespace asio2
{

	template<class _connection_impl_t>
	class tcps_auto_client_impl : public tcps_client_impl<_connection_impl_t>
	{
	public:

		/**
		 * @construct
		 */
		explicit tcps_auto_client_impl(
			std::shared_ptr<url_parser>        url_parser_ptr,
			std::shared_ptr<listener_mgr>      listener_mgr_ptr,
			boost::asio::ssl::context::method  method,
			boost::asio::ssl::context::options options
		)
			: tcps_client_impl<_connection_impl_t>(url_parser_ptr, listener_mgr_ptr, method, options)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~tcps_auto_client_impl()
		{
		}

	};

}

#endif // !__ASIO2_TCPS_AUTO_CLIENT_IMPL_HPP__
