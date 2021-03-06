#include "Game/packet.h"
#include "Cryptography/base_crypto.h"
#include "Cryptography/login.h"
#include "Cryptography/game.h"
#include "Tools/socket.h"
#include "Tools/utils.h"


namespace Net
{
	Factory* Factory::_instance = nullptr;

	Packet* Factory::make(PacketType type, Utils::Game::Session* session, NString data)
	{
		Packet* packet = nullptr;
		if (!_pool.empty())
		{
			packet = _pool.front();
			_pool.pop_front();
		}
		else
		{
			packet = new Packet();
		}

		packet->setType(type);
		packet->setData(data);
		packet->setSession(session);

		switch (type)
		{
			case PacketType::CLIENT_LOGIN:
				packet->setCrypto(Crypto::Client::Login::Encrypter::get(), Crypto::Client::Login::Decrypter::get());
				break;
			case PacketType::CLIENT_GAME:
				packet->setCrypto(Crypto::Client::Game::Encrypter::get(), Crypto::Client::Game::Decrypter::get());
				break;
			case PacketType::SERVER_LOGIN:
				packet->setCrypto(Crypto::Server::Login::Encrypter::get(), Crypto::Server::Login::Decrypter::get());
				break;
			case PacketType::SERVER_GAME:
				packet->setCrypto(Crypto::Server::Game::Encrypter::get(), Crypto::Server::Game::Decrypter::get());
				break;
			default:
				break;
		}

		return packet;
	}

	void Factory::recycle(Packet* packet)
	{
#ifndef NDEBUG
		for (auto it = _pool.begin(); it != _pool.end(); ++it)
		{
			if (*it == packet)
			{
				assert(false && "Packet can not be recycled twice");
			}
		}
#endif

		_pool.push_back(packet);
	}


	Packet::Packet(NString packet):
		Packet()
	{
		_packet = packet;
	}

	Packet::Packet(int size):
		Packet()
	{
		// FIXME: NString does not have a reserve operator
		//_packet.reserve(size);
	}

	Packet::Packet():
		_isCommitted(false),
		_isFinished(false)
	{}

	Packet::~Packet()
	{}

	void Packet::commit()
	{
		if (!_isCommitted)
		{
			// Add session alive if any
			if (_type == PacketType::CLIENT_GAME && _session)
			{
				// TODO: Optimze buffer copying
				_packet = NString() << _session->alive() << ' ' << _packet.get();
			}

			_isCommitted = true;
			_crypter->commit(_packet);
		}
	}

	void Packet::finish()
	{
		if (!_isFinished)
		{
			commit();
			_isFinished = true;
			_crypter->finish(_packet, _session);
		}
	}

	void Packet::send(Socket* socket)
	{
		finish();
		socket->send(_packet);
	}

	std::vector<NString> Packet::decrypt()
	{
		return _decrypter->parse(_packet, _session);
	}

	void Packet::setType(PacketType type)
	{
		_type = type;
	}

	void Packet::setData(NString data)
	{
		_packet = data;
		_isCommitted = false;
		_isFinished = false;
	}

	void Packet::setSession(Utils::Game::Session* session)
	{
		_session = session;
	}

	void Packet::setCrypto(Crypto::Base::Encrypter* crypter, Crypto::Base::Decrypter* decrypter)
	{
		_crypter = crypter;
		_decrypter = decrypter;
	}

	Packet* Packet::clone(Utils::Game::Session* session)
	{
		return gFactory->make(_type, session, _packet.clone());
	}

	Packet& Packet::operator<<(Packet& packet)
	{
		return *this << packet._packet;
	}

	Packet& Packet::operator<<(NString str)
	{
		_packet << fmt::BasicStringRef<char>(str.get(), str.length());
		return *this;
	}

	Packet* operator+(Packet& one, Packet& other)
	{
		Packet* packet = gFactory->make(one.type(), one.session());

		bool useOne = one.data().length() > 0;
		bool useOther = other.data().length() > 0;

		// Commit all packets
		if (useOne)
		{
			one.commit();
			one.finish();
		}

		if (useOther)
		{
			other.commit();
			other.finish();
		}

		// Add them together
		if (useOne)
		{
			*packet << one;
		}

		if (useOther)
		{
			*packet << other;
		}

		// Force finish
		packet->forceFinish();

		// Recycle
		gFactory->recycle(&one);
		gFactory->recycle(&other);

		return packet;
	}
}
