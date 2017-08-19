#include "CircularBuffer.h"
#include "Linker.h"

using namespace std;

bool random;

void ProcessProducer(size_t count, size_t & bufferSize, DWORD delay, size_t msgSize);
void ProcessConsumer(size_t count, size_t & bufferSize, DWORD delay);

void gen_random(char *s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (auto i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}

int main(int arg, char* argM[])
{
	srand(time(NULL));

	DWORD delay = atoi(argM[2]);
	size_t bufferSize = atoi(argM[3]) * 1 << 20;
	size_t numMsg = atoi(argM[4]);
	size_t msgSize = 0;

	if (strcmp("random", argM[5]) == 0)
		random = true;
	else
	{
		random = false;
		msgSize = atoi(argM[5]);
	}

	if (strcmp("producer", argM[1]) == 0)
	{
		ProcessProducer(numMsg, bufferSize, delay, msgSize);

	}
	if (strcmp("consumer", argM[1]) == 0)
	{
		ProcessConsumer(numMsg, bufferSize, delay);

	}

	return 0;
}

void ProcessProducer(size_t count, size_t &bufferSize, DWORD delay, size_t msgSize)
{
	size_t chunkSize = 256;
	size_t maxMsgSize = bufferSize / 4;	//has to be smaller than a quarter of the buffersize
	char *msg = new char[maxMsgSize];
	int countPush = 0;
	
	Circ_Buffer circlBuff = Circ_Buffer(L"produce", bufferSize, true, chunkSize);

	// make a while loop that pushes and pops from the buffer
	// until certain number of messages have been sent.
	// and then finish.
	while (countPush < count)
	{
		if (delay != 0)
			Sleep(delay);

		size_t sizeRandOrFixed = msgSize;

		if (random)
		{
			sizeRandOrFixed = rand() % maxMsgSize + 1;
		}

		gen_random((char*)msg, sizeRandOrFixed);
		
		while (true)
		{
			if (circlBuff.Push(msg, sizeRandOrFixed, chunkSize))
			{	
				printf("%d %s\n", countPush, msg);
				countPush++;
				break;
			}
			else
			{
				Sleep(1);
			}
		}
		
	};
	Sleep(1000);

	delete[]msg;
}

void ProcessConsumer(size_t count, size_t & bufferSize, DWORD delay)
{
	size_t chunkSize = 0;
	size_t maxMsgSize = bufferSize / 4;
	char *msg = new char[maxMsgSize];
	int countPop = 0;
	size_t size0 = 0;

	Circ_Buffer circlBuff = Circ_Buffer(L"consume", bufferSize, false, chunkSize);
	
	while (countPop < count)
	{ 
		if (delay != 0)
			Sleep(delay);

		memset(msg, '\0', maxMsgSize);
		
		if (circlBuff.Pop(msg, size0))
		{
			printf("%d %s\n", countPop, msg);
			countPop++;
		}
		else
		{
			Sleep(1);
		}
	};

	delete[] msg;
}
