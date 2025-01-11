Difference between Terminal and Terminal Emulators:
    - a terminal is a real, physical keyboard/monitor interface used to interact with a computer. Terminal emulators are computer programs that simulates a real terminal.

shell = command line interpreter, it interprets and executes commands typed in the terminal

tty = device file that represents an existing terminal? 
- used to interact with the shell


write vs printf
- write is a sys call, and is at the lowest level. It only writes sequence of bytes to stdout
- printf will call write under the hood. It allows more flexibility (sending strings, floats, etc..) without requiring the user to convert them to bytes.


char* buf vs char buf[]
- what is the difference between pointer and array
    https://c-faq.com/aryptr/aryptrequiv.html
    * in short, pointers != arrays. Their equivalence comes in when performing arithmetic on them. Also, in expressions, the name of the array "decays" into a pointer (it will act as a pointer to the first element of the array).

- when to use * vs [] for a variable-length container of items? 
    https://c-faq.com/struct/structhack.html
    https://stackoverflow.com/questions/18001710/are-flexible-array-members-really-necessary
    * in C99 and onwards, prefer to use flexible array member because they can be stored together within the same block of memory as the structure themselves and we only need to call free() once. When we use pointer, the contigous property of the structure is removed, and we may have to call free() twice to free the structure and the object pointed to by the pointer.

confused about memory in general, stack vs heap ...
- objects stored on the stack are have automatic storage, meaning that they don't have to be manually freed. Objects on the heap needed to be explicitly freed.
- in most cases, store things on the stack. only store on the heap only if needed (most of the time, we store on the heap is because we want to keep the objects longer than the scope its declared in?)
- objects stored on the heap have flexible lifetimes which is important when creating data structures such as binary trees or linked lists.

what is size_t? why use size_t over int or long etc...?










