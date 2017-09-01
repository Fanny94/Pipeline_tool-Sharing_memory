# Pipeline_tool-Sharing_memory
Program where several procesess communicates through a sharing memory area. 
A circular buffer is used to control the shared memory and a filemapping object is created that allows the two processes to read and write to the same memory.

In this project there are one producer, pushing messages and several consumers recieving the messages. The messages in this case could be printed letters or numbers. The program allows to choose if the messages should be fixed or randomized.
