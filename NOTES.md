
Both executed only in the thread where run() is called. [documentation]

Post - 	non-blocking call. Function can't be executed inside the function where it was posted.

Dispatch -  may block the function where it was dispatched.

The `dispatch()` function can be invoked from the current
worker thread, while the `post()` function has to wait until the handler of the worker is complete
before it can be invoked. In other words, the `dispatch()` function's events can be executed from the
current worker thread even if there are other pending events queued up, while the `post()` function's
events have to wait until the handler completes the execution before being allowed to be executed.

`io_context::poll()` - same as `run()` but execute only already READY handles => so no blocking used

`io_context::run()` - 	blocks execution (wait for and execute for handles that are NOT ready yet), i.e.
          blocks until all work has finished.

`strand` - A strand is defined as a strictly sequential invocation of event handlers 
          (i.e. no concurrent invocation). Use of strands allows execution of code 
          in a multithreaded program without the need for explicit locking (e.g. using mutexes). 

`boost::asio::post()` without explicity stated executor such as `io_context`, `strand` (strand is constructed with executor context) will use `system_executor`. Meanwhile `system_executor` will use system's internal threads so data race can be observed if programmer ignore this fact. (See documentation)[https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.associated_i_o_executor]

Example #1. Using system's executer:
`boost::asio::post([](){}); // call with only callable object: function`

`boost::asio::async_read*` can be called with `boost::asio::streambuf` as buffer. In this case note that this buffer will be committed inside `boost::asio::async_read*` function. The programmer only need to consume the used number of bytes.