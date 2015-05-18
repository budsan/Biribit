#include <Biribit/Packet.h>
#include <cstring>
#include <cwchar>

#if defined(SYSTEM_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////
Packet::Packet() :
m_readPos(0),
m_isValid(true)
{
}


////////////////////////////////////////////////////////////
Packet::Packet(Packet&& packet) :
m_readPos(packet.m_readPos),
m_isValid(packet.m_isValid)
{
	packet.m_readPos = 0;
	packet.m_isValid = true;
	m_data = std::move(packet.m_data);
}


////////////////////////////////////////////////////////////
Packet::~Packet()
{

}


////////////////////////////////////////////////////////////
Packet& Packet::operator=(Packet&& packet)
{
	m_readPos = packet.m_readPos;
	m_isValid = packet.m_isValid;

	packet.m_readPos = 0;
	packet.m_isValid = true;
	m_data = std::move(packet.m_data);

	return *this;
}


////////////////////////////////////////////////////////////
Packet::Packet(const void* data, std::size_t sizeInBytes) :
m_readPos(0),
m_isValid(true)
{
	append(data, sizeInBytes);
}


////////////////////////////////////////////////////////////
void Packet::append(const void* data, std::size_t sizeInBytes)
{
	if (data && (sizeInBytes > 0))
	{
		std::size_t start = m_data.size();
		m_data.resize(start + sizeInBytes);
		std::memcpy(&m_data[start], data, sizeInBytes);
	}
}


////////////////////////////////////////////////////////////
void Packet::clear()
{
	m_data.clear();
	m_readPos = 0;
	m_isValid = true;
}


////////////////////////////////////////////////////////////
const void* Packet::getData() const
{
	return !m_data.empty() ? &m_data[0] : NULL;
}


////////////////////////////////////////////////////////////
std::size_t Packet::getDataSize() const
{
	return m_data.size();
}


////////////////////////////////////////////////////////////
bool Packet::endOfPacket() const
{
	return m_readPos >= m_data.size();
}


////////////////////////////////////////////////////////////
std::size_t Packet::getReadPos() const
{
	return m_readPos;
}


////////////////////////////////////////////////////////////
Packet::operator BoolType() const
{
	return m_isValid ? &Packet::checkSize : NULL;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(bool& data)
{
	std::uint8_t value;
	if (*this >> value)
		data = (value != 0);

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::int8_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = *reinterpret_cast<const std::int8_t*>(&m_data[m_readPos]);
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::uint8_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = *reinterpret_cast<const std::uint8_t*>(&m_data[m_readPos]);
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::int16_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = ntohs(*reinterpret_cast<const std::int16_t*>(&m_data[m_readPos]));
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::uint16_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = ntohs(*reinterpret_cast<const std::uint16_t*>(&m_data[m_readPos]));
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::int32_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = ntohl(*reinterpret_cast<const std::int32_t*>(&m_data[m_readPos]));
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::uint32_t& data)
{
	if (checkSize(sizeof(data)))
	{
		data = ntohl(*reinterpret_cast<const std::uint32_t*>(&m_data[m_readPos]));
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(float& data)
{
	if (checkSize(sizeof(data)))
	{
		data = *reinterpret_cast<const float*>(&m_data[m_readPos]);
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(double& data)
{
	if (checkSize(sizeof(data)))
	{
		data = *reinterpret_cast<const double*>(&m_data[m_readPos]);
		m_readPos += sizeof(data);
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(char* data)
{
	// First extract string length
	std::uint32_t length = 0;
	*this >> length;

	if ((length > 0) && checkSize(length))
	{
		// Then extract characters
		std::memcpy(data, &m_data[m_readPos], length);
		data[length] = '\0';

		// Update reading position
		m_readPos += length;
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::string& data)
{
	// First extract string length
	std::uint32_t length = 0;
	*this >> length;

	data.clear();
	if ((length > 0) && checkSize(length))
	{
		// Then extract characters
		data.assign(&m_data[m_readPos], length);

		// Update reading position
		m_readPos += length;
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(wchar_t* data)
{
	// First extract string length
	std::uint32_t length = 0;
	*this >> length;

	if ((length > 0) && checkSize(length * sizeof(std::uint32_t)))
	{
		// Then extract characters
		for (std::uint32_t i = 0; i < length; ++i)
		{
			std::uint32_t character = 0;
			*this >> character;
			data[i] = static_cast<wchar_t>(character);
		}
		data[length] = L'\0';
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator >>(std::wstring& data)
{
	// First extract string length
	std::uint32_t length = 0;
	*this >> length;

	data.clear();
	if ((length > 0) && checkSize(length * sizeof(std::uint32_t)))
	{
		// Then extract characters
		for (std::uint32_t i = 0; i < length; ++i)
		{
			std::uint32_t character = 0;
			*this >> character;
			data += static_cast<wchar_t>(character);
		}
	}

	return *this;
}

////////////////////////////////////////////////////////////
Packet& Packet::read(void* data, std::size_t sizeInBytes)
{
	if ((sizeInBytes > 0) && checkSize(sizeInBytes))
	{
		// Then extract characters
		std::memcpy(data, &m_data[m_readPos], sizeInBytes);

		// Update reading position
		m_readPos += sizeInBytes;
	}

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(bool data)
{
	*this << static_cast<std::uint8_t>(data);
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::int8_t data)
{
	append(&data, sizeof(data));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::uint8_t data)
{
	append(&data, sizeof(data));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::int16_t data)
{
	std::int16_t toWrite = htons(data);
	append(&toWrite, sizeof(toWrite));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::uint16_t data)
{
	std::uint16_t toWrite = htons(data);
	append(&toWrite, sizeof(toWrite));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::int32_t data)
{
	std::int32_t toWrite = htonl(data);
	append(&toWrite, sizeof(toWrite));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(std::uint32_t data)
{
	std::uint32_t toWrite = htonl(data);
	append(&toWrite, sizeof(toWrite));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(float data)
{
	append(&data, sizeof(data));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(double data)
{
	append(&data, sizeof(data));
	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(const char* data)
{
	// First insert string length
	std::uint32_t length = std::strlen(data);
	*this << length;

	// Then insert characters
	append(data, length * sizeof(char));

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(const std::string& data)
{
	// First insert string length
	std::uint32_t length = static_cast<std::uint32_t>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0)
		append(data.c_str(), length * sizeof(std::string::value_type));

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(const wchar_t* data)
{
	// First insert string length
	std::uint32_t length = std::wcslen(data);
	*this << length;

	// Then insert characters
	for (const wchar_t* c = data; *c != L'\0'; ++c)
		*this << static_cast<std::uint32_t>(*c);

	return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator <<(const std::wstring& data)
{
	// First insert string length
	std::uint32_t length = static_cast<std::uint32_t>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0)
	{
		for (std::wstring::const_iterator c = data.begin(); c != data.end(); ++c)
			*this << static_cast<std::uint32_t>(*c);
	}

	return *this;
}


////////////////////////////////////////////////////////////
bool Packet::checkSize(std::size_t size)
{
	m_isValid = m_isValid && (m_readPos + size <= m_data.size());

	return m_isValid;
}


////////////////////////////////////////////////////////////
const void* Packet::onSend(std::size_t& size)
{
	size = getDataSize();
	return getData();
}


////////////////////////////////////////////////////////////
void Packet::onReceive(const void* data, std::size_t size)
{
	append(data, size);
}
