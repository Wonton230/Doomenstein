#include "ActorHandle.hpp"

const ActorHandle ActorHandle::INVALID = ActorHandle(0x0000ffffu, 0x0000ffffu);

ActorHandle::ActorHandle()
{
	m_data = 0x00000000u;
}

ActorHandle::ActorHandle(unsigned int uid, unsigned int index)
{
	unsigned int maskedIndex = 0x0000ffffu & index;
	m_data = uid;
	m_data = m_data << 16;
	m_data = m_data | maskedIndex;
}

bool ActorHandle::IsValid() const
{
	if (*this == ActorHandle::INVALID)
	{
		return false;
	}
	else if (GetIndex() > ActorHandle::MAX_ACTOR_INDEX)
	{
		return false;
	}
	return false;
}

unsigned int ActorHandle::GetIndex() const
{
	return m_data & 0x0000ffffu;
}

bool ActorHandle::operator==(const ActorHandle& other) const
{
	if (m_data == other.m_data)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ActorHandle::operator!=(const ActorHandle& other) const
{
	if (m_data != other.m_data)
	{
		return true;
	}
	else
	{
		return false;
	}
}
