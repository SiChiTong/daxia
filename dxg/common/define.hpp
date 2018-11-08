/*!
 * Licensed under the MIT License.See License for details.
 * Copyright (c) 2018 �콭��Ĵ�Ϻ.
 * All rights reserved.
 *
 * \file define.hpp
 * \author �콭��Ĵ�Ϻ
 * \date ���� 2018
 *
 */

#ifndef __DAXIA_DXG_COMMON_DEFINE_HPP
#define __DAXIA_DXG_COMMON_DEFINE_HPP

namespace daxia
{
	namespace dxg
	{
		namespace common
		{
			typedef unsigned char byte;

			enum Protocol
			{
				Protocol_TCP,
				Protocol_UDP,
				Protocol_Websocket,
				Protocol_HTTP
			};

			enum DefMsgID
			{
				DefMsgID_Connect = -1,
				DefMsgID_DisConnect = -2,
				DefMsgID_Heartbeat = -3
			};

			enum 
			{
				AutoReconnectInterval = 3000
			};
		}

		namespace server
		{
			namespace common = daxia::dxg::common;
		}

		namespace client
		{
			namespace common = daxia::dxg::common;
		}
	}
}


#endif // !__DAXIA_DXG_COMMON_DEFINE_HPP
