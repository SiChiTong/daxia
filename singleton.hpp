/*!
 * Licensed under the MIT License.See License for details.
 * Copyright (c) 2018 漓江里的大虾.
 * All rights reserved.
 *
 * \file sigleton.hpp
 * \author 漓江里的大虾
 * \date 十一月 2018
 * 
 * 提供单实例功能
 *
 */

#ifndef __DAXIA_SINGLETON_HPP
#define __DAXIA_SINGLETON_HPP

namespace daxia
{
	template<class T>
	class singleton
	{
	protected:
		singleton() = delete;
		singleton(const singleton&) = delete;
		singleton& operator=(const singleton&) = delete;
	public:
		~singleton(){}
	public:
		static T& Instance()
		{
			static T v;
			return v;
		}
	};
}

#endif // !__DAXIA_SINGLETON_HPP
