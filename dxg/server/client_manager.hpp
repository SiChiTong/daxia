/*!
 * Licensed under the MIT License.See License for details.
 * Copyright (c) 2018 �콭��Ĵ�Ϻ.
 * All rights reserved.
 *
 * \file client_manager.hpp
 * \author �콭��Ĵ�Ϻ
 * \date ���� 2018
 * 
 */

#ifndef __DAXIA_DXG_SERVER_CLIENT_MANAGER_HPP
#define __DAXIA_DXG_SERVER_CLIENT_MANAGER_HPP

#include <daxia/dxg/server/scheduler.hpp>

namespace daxia
{
	namespace dxg
	{
		namespace server
		{
			// �ͻ��˹�����
			class ClientManager
			{
			public:
				typedef std::shared_ptr<Client> client_ptr;
				typedef std::shared_ptr<ClientManager> clientMgr_ptr;
				typedef std::lock_guard<std::mutex> lock_guard;
			public:
				ClientManager(Scheduler& scheduler);
				~ClientManager(){}
			public:
				// ����һ���ͻ���
				void AddClient(client_ptr c);

				// ɾ��һ���ͻ���
				void DeleteClient(long long id);

				// ��ȡ�ͻ���
				client_ptr GetClient(long long id);

				// �����Զ������ݻ�ȡ�ͻ���
				client_ptr GetClientByUserData(const std::string& key, const common::shared_buffer& data);

				// �㲥һ����Ϣ
				void Broadcast();

				// ����һ���ͻ�����
				void CreateGroup(const std::string& key);

				// ��ȡ�ͻ�����
				clientMgr_ptr GetGroup(const std::string& key);

				// ����/�ر��������
				void EnableCheckHeartbeat(unsigned long interval);
			private:
				std::map<long long, client_ptr> clients_;
				std::map<std::string, clientMgr_ptr> group_;
				std::mutex clientsLocker_;
				std::mutex groupLocker_;
				Scheduler& scheduler_;
				long long schedulerID_;
			};

			//////////////////////////////////////////////////////////////////////////
			ClientManager::ClientManager(Scheduler& scheduler)
				: scheduler_(scheduler)
				, schedulerID_(-1)
			{
			}

			void ClientManager::AddClient(client_ptr c)
			{
				lock_guard locker(clientsLocker_);

				clients_[c->GetClientID()] = c;
			}

			void ClientManager::DeleteClient(long long id)
			{
				lock_guard locker(clientsLocker_);

				clients_.erase(id);
			}

			ClientManager::client_ptr ClientManager::GetClient(long long id)
			{
				client_ptr client;

				lock_guard locker(clientsLocker_);

				auto iter = clients_.find(id);
				if (iter != clients_.end())
				{
					client = iter->second;
				}

				return client;
			}

			ClientManager::client_ptr ClientManager::GetClientByUserData(const std::string& key, const common::shared_buffer& data)
			{
				client_ptr client;

				lock_guard locker(clientsLocker_);

				for (auto clientIter = clients_.begin();
					clientIter != clients_.end();
					++clientIter)
				{
					auto userDataIter = clientIter->second->userData_.find(key);
					if (userDataIter != clientIter->second->userData_.end() && userDataIter->second.size() == data.size() && !data.empty())
					{
						const common::byte* p1 = userDataIter->second.get();
						const common::byte* p2 = data.get();

						if (memcmp(p1, p2, data.size()) == 0)
						{
							client = clientIter->second;
							break;
						}
					}
				}

				return client;
			}

			void ClientManager::CreateGroup(const std::string& key)
			{
				lock_guard locker(groupLocker_);

				auto iter = group_.find(key);
				if (iter == group_.end())
				{
					group_[key] = clientMgr_ptr(new ClientManager(scheduler_));
				}
			}

			ClientManager::clientMgr_ptr ClientManager::GetGroup(const std::string& key)
			{
				lock_guard locker(groupLocker_);

				clientMgr_ptr mgr;

				auto iter = group_.find(key);
				if (iter != group_.end())
				{
					mgr = iter->second;
				}

				return mgr;
			}

			void ClientManager::EnableCheckHeartbeat(unsigned long interval)
			{
				if (interval == 0 && schedulerID_ != -1)
				{
					// �ر��������
					scheduler_.Unschedule(schedulerID_);
				}
				else
				{
					// �����������
					schedulerID_ = scheduler_.Schedule([&,interval]()
					{
						using namespace std::chrono;

						time_point<system_clock, milliseconds> now = time_point_cast<milliseconds>(system_clock::now());

						lock_guard locker(clientsLocker_);

						for (auto iter = clients_.begin(); iter != clients_.end(); ++iter)
						{
							if ((now - iter->second->lastReadTime_).count() >= interval)
							{
								iter->second->Close();
							}
						}
					}, 2000);
				}
			}

		}// namespace server
	}// namespace dxg
}// namespace daxia

#endif // !__DAXIA_DXG_SERVER_CLIENT_MANAGER_HPP
