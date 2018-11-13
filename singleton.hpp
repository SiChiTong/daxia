/*!
 * Licensed under the MIT License.See License for details.
 * Copyright (c) 2018 �콭��Ĵ�Ϻ.
 * All rights reserved.
 *
 * \file sigleton.hpp
 * \author �콭��Ĵ�Ϻ
 * \date ʮһ�� 2018
 * 
 * �ṩ��ʵ������
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
