Circular Buffer Code Project
Included Files:

Code files-
CircularBuffer.h
CircularBuffer.cpp
SingleBlockPool.h
SingleBlockPool.cpp
packetdefs.h
logger.h

Unit tests-
BufferTest.cpp

Unix make file-
Makefile 

Windows solution-
RippleBuffer.sln
RippleBuffer.vcxproj

All code files, with the exception BufferTest.cpp, are required to compile the CircularBuffer class.
BufferTest.cpp is a rudimentary unit test for the class that I put together.  The unit tests include
one non-unit test for checking the multi-threading capability which, if desired, should be un-commented 
before running. I wasn't sure what unit test framework you use and so I created a generic 
class to test the code which, in my experience, is part of a peer code review.  I have also included
two project files, the Makefile for unix and Windows Visual Studio 2017 solution files.  I verified
that it compiled and ran on both environments with the flags specified in the buffer_rules document.

I decided to break the task up into two parts, the CircularBuffer itself and a memory manager
which I call SingleBlockPool.  CircularBuffer guarantees the FIFO order of the packets and 
SingleBlockPool allows out-of-order allocation of the memory as needed to help mitigate 
fragmentation of the memory block.  It first looks at the front and back of the queue for space
and then looks between each block for the space required.

Because of the need for 100% up-time I decided that the only function to throw exceptions 
would be the enqueue.  enqueue() is defined as a void function so if something goes wrong
the caller is left not knowing what went wrong.  That is why I decided to throw exceptions
in this case so that the caller can decide what to do if the function failed.  If there is no 
memory to store the packet then the queue can't do anything with it so the caller can decide if
they want to buffer it, dequeue before trying to queue the new packet or simply let the packet go.
I define an enum of 3 types of failure that it can throw: CANNOT_OBTAIN_MEMORY, INVALID_PACKET and 
INVALID_SIZED_PACKET. If something goes wrong in any of the other functions it is less significant
than if a packet can't be added so it is logged and the class maintains a safe running state.

Along with the exceptions I also created a logger (logger.h) that can output to file, cout or another ostream
so someone can monitor the progress of the queue.  The logger is very rudimentary as I focused
on the main problem first also knowing a logger is typically already made.  However I felt it necessary
to allow monitoring of the queue so memory allocation can be adjusted as needed and one can be alerted of issues that may arise.

Finally, packetdefs.h defines the packets specified in the doc plus a couple others that I used
for testing purposes.

