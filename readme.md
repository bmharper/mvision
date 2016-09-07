Memory Leaks
------------
OpenCV leaks memory just by linking to it. It performs a bunch of static C++ allocations when it's
DLL is loaded. I can't find a way of cleaning that up.