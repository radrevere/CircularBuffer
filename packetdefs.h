#pragma once
#include <cstdint>
// this is the data header definition
struct packet
{
	uint32_t size;
	uint32_t id;
};


// testing packets as specified by Ripple buffer_rules.pdf
// these guarantee the packet header integrity
struct point_packet
{
	uint32_t size;
	uint32_t id;
	double x;
	double y;
	double z;
};

struct string_packet
{
	struct point_packet header;
	char buf[256];
};

struct point_packet_2d
{
	uint32_t size;
	uint32_t id;
	double x;
	double y;
};

struct health_packet
{
	struct packet header;
	uint32_t status;
	uint32_t type;
};

// not byte aligned for testing incorrect size
#pragma pack(push,1)
struct invalid_packet
{
	struct packet header;
	char buf[11];
};
#pragma pack(pop)

