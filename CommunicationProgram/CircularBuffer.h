#pragma once
#include "Linker.h"

using namespace std;

class Circ_Buffer
{
private:

	struct MessageHeader
	{
		size_t messageID;
		size_t length;
		size_t padding;
		size_t numberOfConsumers;
	};

	char* messageData;
	size_t* controlData;
	size_t* head;
	size_t* tail;
	size_t* freememory;
	size_t bufferSize;
	size_t* clients;

	int countId;
	bool canRead;
	size_t internalTail;

	HANDLE fMapMessageData;
	HANDLE fMapControlData;

public:

	Circ_Buffer(LPCWSTR bufferName,
				size_t & bufferSize, //size of filemap
				bool isProducer, //to check if its a producer or not		
				size_t& chunkSize); //round up messages to the multiple of this size

	~Circ_Buffer();

	//send message to buffer, if it returns true the message succeded
	//if there isn´t enough space in the buffer it should return false
	bool Push(const char* message, size_t length, size_t chunkSize);
	//try to read message from buffer
	bool Pop(char* message, size_t& length);
	//size_t random(size_t min, size_t max);
	//void randomString(char * s, const size_t maxSize);
};

