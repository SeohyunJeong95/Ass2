# EECS 3221 Assingment2 
1. First copy the files "alarm_mutex.c", and "errors.h" into your
   own directory.

2. To compile the program "alarm_mutex.c", use the following command:

      cc alarm_mutex.c -D_POSIX_PTHREAD_SEMANTICS -lpthread

3. Type "a.out" to run the executable code.

4. At the prompt "ALARM>", type in the number of seconds at which
   the alarm should expire, followed by the text of the message.
   For example:

   ALARM> 2 Good Morning!

  (To exit from the program, type Ctrl-d.)

5.. Read pages 52-58 of the book "Programming with POSIX Threads"
   by David R. Butenhof for a detailed explanation of how the
   program "alarm_mutex.c" works.
   (The book "Programming with POSIX Threads" has been put on
   reserve in Steacie Library.)
