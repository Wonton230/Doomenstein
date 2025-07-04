#pragma once

struct ActorHandle
{
public:
	ActorHandle();
	ActorHandle(unsigned int uid, unsigned int index);

	bool IsValid() const;
	unsigned int GetIndex() const;
	bool operator==(const ActorHandle& other) const;
	bool operator!=(const ActorHandle& other) const;

	static const ActorHandle INVALID;
	static const unsigned int MAX_ACTOR_UID = 0x0000fffeu;
	static const unsigned int MAX_ACTOR_INDEX = 0x0000ffffu;

private:
	unsigned int m_data;
};	