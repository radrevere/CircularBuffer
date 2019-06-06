// RippleBuffer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include "packetdefs.h"
#include "CircularBuffer.h"
#include "logger.h"

using namespace std;

void initStrPck(string_packet *sp)
{
	memset(sp, 0x11, sizeof(string_packet));
	sp->buf[255] = 0x7F;
	sp->buf[0] = 0x7F;
	sp->header.size = sizeof(string_packet);
}
void setupStrPck(string_packet *sp, uint32_t id, string str)
{
	sp->header.size = sizeof(string_packet);
	sp->header.id = id;
	memset(sp->buf, 0, 256);
	strncpy(sp->buf, str.c_str(),256);
}

void initPtPck(point_packet *pp)
{
	memset(pp, 0x22, sizeof(point_packet));
	pp->size = sizeof(point_packet);
	pp->z = 1;
}
void setupPtPck(point_packet *pp, uint32_t id, double x, double y, double z)
{
	pp->size = sizeof(point_packet);
	pp->id = id;
	pp->x = x;
	pp->y = y;
	pp->z = z;
}

size_t testNumber;

class TestCircularBuffer {
	string strLogging;
	CircularBuffer *cbuf;
	uint32_t ppacketSize;
	uint32_t spacketSize;
	size_t bufferSize;

public:
	TestCircularBuffer()
	{
		ppacketSize = sizeof(point_packet);
		spacketSize = sizeof(string_packet);
		bufferSize = spacketSize * 50;
		cbuf = new CircularBuffer(bufferSize);
	}
	~TestCircularBuffer()
	{
		if (cbuf)
		{
			delete cbuf;
		}
	}

	// test the addition of a packet to the buffer
	bool TestAdd()
	{
		point_packet pp;
		setupPtPck(&pp, 1, 5, 10, 15);
		cbuf->enqueue((const packet*)&pp,pp.size);
		packet *pkt = cbuf->find([](packet *p) {return p->id == 1; });
		if (!pkt)
		{
			return false;
		}
		point_packet *pPP = (point_packet*)pkt;
		if (pPP->x != 5 || pPP->y != 10 || pPP->z != 15)
		{
			return false;
		}
		if (cbuf->count() != 1)
		{
			return false;
		}
		cbuf->dequeue();
		if (cbuf->count() != 0)
		{
			return false;
		}
		return true;
	}

	// test the reaction to an invalid packet
	bool TestInvalidPacket()
	{
		invalid_packet pp;
		memset(&pp, 0, sizeof(invalid_packet));
		pp.header.size = sizeof(invalid_packet);
		try {
			cbuf->enqueue((const packet*)&pp, pp.header.size);
		}
		catch (BUFFER_ERROR e)
		{
			cout << "Exception thrown: " << e;
			if (e == INVALID_SIZED_PACKET)
			{
				cout << " VALID" << endl;
				return true;
			}
			cout << " INVALID" << endl;
		}
		cbuf->clearBuffer();
		return false;

	}

	// test the find functionality
	bool TestFind()
	{
		strLogging = "";
		stringstream outputString(strLogging);
		point_packet pkt;
		initPtPck(&pkt);
		for (int i = 0; i < 10; i++)
		{
			pkt.id = i;
			cbuf->enqueue((const packet*)&pkt, pkt.size);
		}
		packet * found = cbuf->find([](packet *p) {return p->id == 5 ? true : false; });
		if (found == nullptr)
		{
			return false;
		}
		if (found->id != 5)
		{
			return false;
		}
		cbuf->clearBuffer();
		return true;
	}

	// test the visit functionality
	bool TestVisit()
	{
		strLogging = "";
		stringstream outputString(strLogging);
		point_packet pkt;
		initPtPck(&pkt);
		for (int i = 0; i < 10; i++)
		{
			pkt.id = i;
			cbuf->enqueue((const packet*)&pkt, pkt.size);
		}
		testNumber = 0;
		cbuf->visit([](packet *p) {if(p->size == sizeof(point_packet))testNumber++; });
		
		if (testNumber != 10)
		{
			return false;
		}
		cbuf->clearBuffer();
		return true;
	}

	// test what happens when the buffer gets full
	bool TestTooManyPackets()
	{
		// calculate too many packets
		const size_t total = (bufferSize/ppacketSize) + 10;
		point_packet pkt;
		initPtPck(&pkt);
		for (size_t i = 0; i < total; i++)
		{
			pkt.id = (uint32_t)i;
			try {
				cbuf->enqueue((const packet*)&pkt, pkt.size);
			}
			catch (BUFFER_ERROR e)
			{
				cbuf->clearBuffer();
				if (e == CANNOT_OBTAIN_MEMORY)
				{
					return true;
				}
				return false;
			}
		}
		cbuf->clearBuffer();
		return false;
	}

	// test the addition of variable size packets
	bool TestVariablePackets()
	{
		health_packet hp;
		hp.header.id = 0;
		hp.header.size = sizeof(health_packet);
		hp.status = 5;
		hp.type = 3;
		point_packet pkt;
		initPtPck(&pkt);
		string_packet sp;
		initStrPck(&sp);

		try {
			cbuf->enqueue((packet*)&hp, hp.header.size);
			cbuf->enqueue((packet*)&pkt, pkt.size);
			cbuf->enqueue((packet*)&sp, sp.header.size);
			pkt.id++;
			cbuf->enqueue((packet*)&pkt, pkt.size);
			sp.header.id++;
			cbuf->enqueue((packet*)&sp, sp.header.size);
			pkt.id++;
			cbuf->enqueue((packet*)&pkt, pkt.size);
			cbuf->dequeue();
			pkt.id++;
			cbuf->enqueue((packet*)&pkt, pkt.size);
			cbuf->dequeue();
			pkt.id++;
			cbuf->enqueue((packet*)&pkt, pkt.size);
		}
		catch (...)
		{
			cbuf->clearBuffer();
			return false;
		}
		if (cbuf->count() != 6)
		{
			cbuf->clearBuffer();
			return false;
		}
		cbuf->clearBuffer();
		return true;
	}

	// test the addition of variable sized packets in a multi-threaded environment
	bool TestMultiThread()
	{
		auto addFunc = [](CircularBuffer *pBuf) {
			point_packet pkt;
			initPtPck(&pkt);
			int startAt = 0;
			for (int i = 0; i < 1000; i++)
			{
				pkt.id = startAt + i;
				try {
					pBuf->enqueue((const packet*)&pkt, pkt.size);
				}
				catch (BUFFER_ERROR e)
				{
					cout << "Caught error: " << e << endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		};
		auto addFunc2 = [](CircularBuffer *pBuf) {
			health_packet pkt;
			pkt.header.id = 0;
			pkt.header.size = sizeof(health_packet);
			pkt.status = 5;
			pkt.type = 3;
			int startAt = 2000;
			for (int i = 0; i < 500; i++)
			{
				pkt.header.id = startAt + i;
				try {
					pBuf->enqueue((const packet*)&pkt, pkt.header.size);
				}
				catch (BUFFER_ERROR e)
				{
					cout << "Caught error: " << e << endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		};
		auto removeFunc = [](CircularBuffer *pBuf) {
			size_t count = 0;
			while (pBuf->count() > 0)
			{
				count++;
				pBuf->dequeue();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		};
		auto visitFunc = [](CircularBuffer *pBuf) {
			while (pBuf->count() > 0)
			{
				pBuf->visit([](packet *p) {
					cout << p->id << endl; });
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
		};
		thread t1(addFunc, cbuf);
		thread t2(removeFunc, cbuf);
		thread t3(addFunc2, cbuf);
		thread t4(visitFunc, cbuf);
		if (t1.joinable())
		{
			t1.join();
		}
		if (t2.joinable())
		{
			t2.join();
		}
		if (t3.joinable())
		{
			t3.join();
		}
		if (t4.joinable())
		{
			t4.join();
		}

		return true;
	}

	void TestAll()
	{
 		runTest(TestAdd(), "Add Test");
		runTest(TestInvalidPacket(), "Invalid Test");
		runTest(TestFind(), "Find Test");
		runTest(TestVisit(), "Visit Test");
		runTest(TestTooManyPackets(), "Too Many Packets");
		runTest(TestVariablePackets(), "Different Packets");
		// uncomment to test multi-threading
		//runTest(TestMultiThread(), "Multi-thread");
	}
private:
	void runTest(bool passed, const char *info)
	{
		cout << info << ": ";
		if (!passed)
		{
			cout << "FAILED";
		}
		else {
			cout << "PASSED";
		}
		cout << endl;
	}
};

int main()
{
	TestCircularBuffer tester;
	tester.TestAll();
}
