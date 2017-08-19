#include "CircularBuffer.h"

class Mutex
{
private:
	HANDLE handle;
public:

	Mutex(const char* name)
	{
		handle = CreateMutex(nullptr, false, (LPCWSTR)name);
	}
	~Mutex()
	{
		ReleaseMutex(handle);
	}
	void Lock(DWORD milliseconds = INFINITE)
	{
		WaitForSingleObject(handle, milliseconds);
	}
	void Unlock()
	{
		ReleaseMutex(handle);
	}
};

Circ_Buffer::Circ_Buffer(LPCWSTR bufferName, size_t & bufferSize, bool isProducer, size_t& chunkSize)
{

	//Create file mapping object for assemble content of a file
	fMapMessageData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, TEXT("filemap"));

	if (fMapMessageData == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), GetLastError());
		exit(0);
	}

	//File view = virtual address space that a process uses to access the file's contents
	//message containing the data 
	messageData = (char*)MapViewOfFile(fMapMessageData, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);

	if (messageData == NULL)
	{
		_tprintf(TEXT("Could not create file mapping view object (%d).\n"),
			GetLastError());

		CloseHandle(fMapMessageData);
		exit(0);
	}

	fMapControlData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, TEXT("controlFilemap"));

	if (fMapControlData == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), GetLastError());
		exit(0);
	}

	controlData = (size_t*)MapViewOfFile(fMapControlData, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);

	if (controlData == NULL)
	{
		_tprintf(TEXT("Could not create file mapping view object (%d).\n"),
			GetLastError());

		CloseHandle(fMapControlData);
		exit(0);
	}

	this->bufferSize = bufferSize;

	head = controlData;
	tail = head + 1;
	freememory = tail + 1;
	clients = freememory + 1;

	if (!GetLastError() == ERROR_ALREADY_EXISTS || !isProducer)
	{
		*head = 0;
		*tail = 0;
		internalTail = 0;
		*freememory = this->bufferSize;
	}

	if (!isProducer)
	{
		*clients+= 1;
	}

	countId = 1;
}

Circ_Buffer::~Circ_Buffer()
{
	UnmapViewOfFile(messageData);
	UnmapViewOfFile(controlData);
	CloseHandle(fMapMessageData);
	CloseHandle(fMapControlData);
}

bool Circ_Buffer::Push(const char * message, size_t length, size_t chunkSize)
{
	size_t reminder = (length + sizeof(MessageHeader)) % chunkSize;
	size_t padding = chunkSize - reminder;

	size_t memLastInBuffer = bufferSize - *head;

	Mutex mutObject = ("myMutex");
	mutObject.Lock();

	if ((length + sizeof(MessageHeader) + padding) >= memLastInBuffer) //if the message is greater than the remaining memory in the end of the buffer 
	{
		if (*tail != 0)
		{
			MessageHeader dummyHeader;
			dummyHeader.length = memLastInBuffer - sizeof(MessageHeader); //substact with header to get the precise length of the last message
			dummyHeader.messageID = 0;
			dummyHeader.padding = 0;
			dummyHeader.numberOfConsumers = *clients;

			//fill last part of the buffer with dummymessage
			//write message
			memcpy(messageData + *head, &dummyHeader, sizeof(MessageHeader));

			*freememory -= (dummyHeader.length + sizeof(MessageHeader) + dummyHeader.padding); //substract length of message from the free memory
			//update head
			*head = 0; //set head to the beginning of te buffer

			mutObject.Unlock();
			return false;
		}
		else
		{
			mutObject.Unlock();
			return false;
		}
	}

	//there is free space in the buffer
	else if ((length + sizeof(MessageHeader) + padding) < (*freememory - 1))
	{
		MessageHeader header;
		header.messageID = countId;
		header.length = length;
		header.padding = padding;

		header.numberOfConsumers = *clients;
	
		//write message
		memcpy(messageData + *head, &header, sizeof(MessageHeader));
		memcpy(messageData + *head + sizeof(MessageHeader), message, header.length);

		Mutex mutObject = ("myMutex");
		mutObject.Lock();
		*freememory -= (header.length + sizeof(MessageHeader) + header.padding); //substract length of message from the free memory

		//update head
		*head = (*head + header.length + sizeof(MessageHeader) + header.padding) % bufferSize;

		mutObject.Unlock();
		countId++;
		return true;
	}

    else 
    {
		mutObject.Unlock();
		return false;
    }

}

bool Circ_Buffer::Pop(char* message, size_t& length)
{
	Mutex mutObject = ("myMutex");
	mutObject.Lock();
    //check if there is something to read
    if (*freememory < bufferSize)
    {   
		if (*head != internalTail)
		{
			MessageHeader *h = ((MessageHeader*)&messageData[internalTail]);
			length = h->length; //copy length of message from header

			//if messageID = 0, then its a dummymessage
			if (h->messageID == 0)
			{
				
				h->numberOfConsumers--;
				if (h->numberOfConsumers == 0)
				{
					*freememory += (length + sizeof(MessageHeader) + h->padding); //set new size of the free memory
					*tail = 0;
				}

				internalTail = 0; //set tail to the beginning of the buffer

				mutObject.Unlock();
				return false;
			}
			else
			{
	
				memcpy(message, &messageData[internalTail + sizeof(MessageHeader)], length); //read message
				internalTail = (internalTail + (length + sizeof(MessageHeader) + h->padding)) % bufferSize; //move internal tail (keeps track of where the next message will be read)


				h->numberOfConsumers--;
				if (h->numberOfConsumers == 0)
				{ 
					*freememory += (length + sizeof(MessageHeader) + h->padding); //set new size of the free memory
					*tail = internalTail; //move the shared tail

				}

				mutObject.Unlock();
				return true;
			}
		}
		else
		{
			mutObject.Unlock();
			return false;
		}
    }
    else
    {
		mutObject.Unlock();
        return false;
    }
	mutObject.Unlock();

}
