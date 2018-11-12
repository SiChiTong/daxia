/*!
 * Licensed under the MIT License.See License for details.
 * Copyright (c) 2018 �콭��Ĵ�Ϻ.
 * All rights reserved.
 *
 * \file client.hpp
 * \author �콭��Ĵ�Ϻ
 * \date ���� 2018
 * 
 */

#ifndef __DAXIA_DXG_SERVER_CLIENT_HPP
#define __DAXIA_DXG_SERVER_CLIENT_HPP

#include <mutex>
#include <thread>
#include <memory>
#include <queue>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <daxia/dxg/common/define.hpp>
#include <daxia/dxg/common/parser.hpp>
#include <daxia/dxg/common/shared_buffer.hpp>

namespace daxia
{
	namespace dxg
	{
		namespace server
		{
			class ClientManager;

			// �ͻ��˲�����
			class Client
			{
				friend ClientManager;
			public:
				typedef boost::asio::ip::tcp::socket socket;
				typedef boost::asio::ip::tcp::endpoint endpoint;
				typedef std::shared_ptr<socket> socket_ptr;
				typedef std::lock_guard<std::mutex> lock_guard;
				typedef std::shared_ptr<Client> client_ptr;
				typedef std::function<void(const boost::system::error_code&, long long, int, common::shared_buffer)> handler;
				typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> timepoint;
			protected:
				Client(socket_ptr sock, common::Parser::parser_ptr parser, handler onMessage,long long id);
			public:
				~Client();
			public:
				// ��ȡ�ͻ���ID
				long long GetClientID() const;

				// �Ͽ�����
				void Close();

				// �����Զ�������
				void SetUserData(const std::string& key, const common::shared_buffer& data);

				// ��ȡ�Զ�������
				void GetUserData(const std::string& key, common::shared_buffer& buff);

				// ɾ��ָ�����Զ�������
				void DeleteUserData(const std::string& key);

				// ɾ�������Զ�������
				void DeleteAllUserData();

				// ��ȡ�ͻ��˵�ַ
				std::string RemoterAddr() const;

				// ������Ϣ
				void WriteMessage(const void* date,int len);
			private:
				void updateHeartbeat();
				void doWriteMessage();
				void onRead(const boost::system::error_code& err, size_t len);
			private:
				std::shared_ptr<socket> sock_;
				endpoint endpoint_;
				std::shared_ptr<common::Parser> parser_;
				long long id_;
				std::map<std::string, common::shared_buffer> userData_;
				std::mutex userDataLocker_;
				common::shared_buffer buffer_;
				handler onMessage_;
				std::mutex writeLocker_;
				std::queue<common::shared_buffer> writeBufferCache_;
				timepoint lastReadTime_;
			};

			//////////////////////////////////////////////////////////////////////////
			inline Client::Client(socket_ptr sock, common::Parser::parser_ptr parser, handler onMessage, long long id)
				: sock_(sock)
				, parser_(parser)
				, buffer_(1024 * 8)
				, onMessage_(onMessage)
				, id_(id)
			{
				updateHeartbeat();

				endpoint_ = sock_->remote_endpoint();

				sock_->async_read_some(buffer_.asio_buffer(), std::bind(&Client::onRead, this, std::placeholders::_1, std::placeholders::_2));
			}

			inline Client::~Client()
			{
				Close();
			}

			inline long long Client::GetClientID() const
			{
				return id_;
			}

			inline void Client::Close()
			{
				sock_->close();
			}

			inline void Client::SetUserData(const std::string& key, const common::shared_buffer& data)
			{
				lock_guard locker(userDataLocker_);

				userData_[key] = data;
			}

			inline void Client::GetUserData(const std::string& key, common::shared_buffer& buff)
			{
				lock_guard locker(userDataLocker_);

				auto iter = userData_.find(key);
				if (iter != userData_.end())
				{
					buff = iter->second;
				}
			}

			inline void Client::DeleteUserData(const std::string& key)
			{
				lock_guard locker(userDataLocker_);

				userData_.erase(key);
			}

			inline void Client::DeleteAllUserData()
			{
				lock_guard locker(userDataLocker_);

				userData_.clear();
			}

			inline std::string Client::RemoterAddr() const
			{
				return (boost::format("%s:%d") % endpoint_.address().to_string() % endpoint_.port()).str();
			}

			inline void Client::WriteMessage(const void* date, int len)
			{
				lock_guard locker(writeLocker_);

				bool isWriting = !writeBufferCache_.empty();

				common::shared_buffer buffer;
				parser_->Marshal(static_cast<const unsigned char*>(date), len, buffer);
				writeBufferCache_.push(buffer);

				if (!isWriting)
				{
					doWriteMessage();
				}
			}

			inline void Client::updateHeartbeat()
			{
				using namespace std::chrono;

				time_point<system_clock, milliseconds> now = time_point_cast<milliseconds>(system_clock::now());

				lastReadTime_ = now;
			}

			inline void Client::doWriteMessage()
			{
				boost::asio::async_write(*sock_, writeBufferCache_.front().asio_buffer(), [&](const boost::system::error_code& ec, std::size_t size)
				{
					lock_guard locker(writeLocker_);

					if (ec)
					{
						// clear writeBufferCache_
						std::queue<common::shared_buffer> empty;
						swap(empty, writeBufferCache_);
					}
					else
					{
						writeBufferCache_.pop();

						if (!writeBufferCache_.empty())
						{
							doWriteMessage();
						}
					}
				});
			}

			inline void Client::onRead(const boost::system::error_code& err, size_t len)
			{
				if (!err)
				{
					// ��ȡ�����İ�ͷ
					buffer_.resize(buffer_.size() + len);
					if (buffer_.size() < parser_->GetPacketHeadLen())
					{
						sock_->async_read_some(buffer_.asio_buffer(buffer_.size()), std::bind(&Client::onRead, this, std::placeholders::_1, std::placeholders::_2));
						return;
					}

					// ������ͷ
					size_t contentLen = 0;
					bool ok = parser_->Unmarshal(buffer_.get(), buffer_.size(),contentLen);
					if (!ok)
					{
						// ��ͷ����ʧ�ܣ����������������½���
						buffer_.clear();
					}
					else
					{
						// ��ͷ�����ɹ���������������
						if (buffer_.size() < parser_->GetPacketHeadLen() + contentLen)
						{
							// ��ȡ����������
							sock_->async_read_some(buffer_.asio_buffer(buffer_.size()), std::bind(&Client::onRead, this, std::placeholders::_1, std::placeholders::_2));
							return;
						}

						// ������
						if (contentLen == 0)
						{
							updateHeartbeat();

							if (onMessage_)
							{
								onMessage_(err, id_, common::DefMsgID_Heartbeat, common::shared_buffer());
							}

							sock_->async_read_some(buffer_.asio_buffer(), std::bind(&Client::onRead, this, std::placeholders::_1, std::placeholders::_2));
							return;
						}

						// ��������
						int msgID = 0;
						common::shared_buffer msg(contentLen);
						bool ok = parser_->Unmarshal(buffer_.get(), buffer_.size(), msgID, msg);
						if (!ok)
						{
							// ���Ľ���ʧ�ܣ����������������½���
							buffer_.clear();
						}
						else
						{
							// ��ͷ�����ɹ����������ݺ��������
							if (buffer_.size() > parser_->GetPacketHeadLen() + contentLen)
							{
								size_t remain = buffer_.size() - (parser_->GetPacketHeadLen() + contentLen);
								for (size_t i = 0; i < remain; ++i)
								{
									*(buffer_.get() + i) = *(buffer_.get() + (parser_->GetPacketHeadLen() + contentLen) + i);
								}

								buffer_.resize(remain);
							}
							else
							{
								buffer_.clear();
							}

							if (onMessage_)
							{
								onMessage_(err, id_, msgID, msg);
							}
						}
					}

					sock_->async_read_some(buffer_.asio_buffer(), std::bind(&Client::onRead, this, std::placeholders::_1, std::placeholders::_2));
				}
				else
				{
					if (onMessage_)
					{
						onMessage_(err, id_, common::DefMsgID_DisConnect, common::shared_buffer());
					}
				}
			}

		}// namespace server
	}// namespace dxg
}// namespace daxia

#endif	// !__DAXIA_DXG_SERVER_CLIENT_HPP