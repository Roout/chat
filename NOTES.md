# Learning notes about ASYNC library

## Post and Dispatch

Both executed only in the thread where run() is called. [see documentation]
**Post** - non-blocking call. Function can't be executed inside the function where it was posted.
**Dispatch** - may block the function where it was dispatched.

The `dispatch()` function can be invoked from the current worker thread, while the `post()` function has to wait until the handler of the worker is complete before it can be invoked. In other words, the `dispatch()` function's events can be executed from the current worker thread even if there are other pending events queued up, while the `post()` function's events have to wait until the handler completes the execution before being allowed to be executed.

`io_context::poll()` - same as `run()` but execute only for already READY handles => so no blocking used

`io_context::run()` - blocks execution (wait for and execute for handles that are NOT ready yet), i.e. blocks until all work has finished.

`strand` - A strand is defined as a strictly sequential invocation of event handlers (i.e. no concurrent invocation). Use of strands allows execution of code in a multithreaded program without the need for explicit locking (e.g. using mutexes).

`boost::asio::post()` without explicity stated executor such as `io_context`, `strand` (strand is constructed with executor context) will use `system_executor`. Meanwhile `system_executor` will use system's internal threads so data race can be observed if programmer ignore this fact. [See documentation](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.associated_i_o_executor)

**Example #1**. Using system's executer:
`boost::asio::post([](){}); // call with only callable object: function`

### Working with asio buffers

`boost::asio::async_read*` can be called with `boost::asio::streambuf` as buffer. In this case note that this buffer will be committed inside `boost::asio::async_read*` function. The programmer only need to consume the used number of bytes.

## Concurrency and thread-safety

**Problems:**

1. Concurrent execution of functions that access the same socket.
2. Concurrent execution of delegates that enclose the same socket.
3. Interleaved execution of delegates that write to the same socket.  
[Good answer](https://stackoverflow.com/a/40588070/11468611)  
[About async_write](https://www.boost.org/doc/libs/1_73_0/doc/html/boost_asio/reference/async_write/overload7.html)  

**Reasons for using strand:**

- strand for callbacks (solve the issue #1)
  - prevent data race around ostream;
  - prevent data race around the buffer, i.e. one OnWrite function hasn't return yet and another one started. However It still won't solve fully problem with buffers if `OnWrite` can't send all message at once!
- wrap `Client::Write(text)` function by strand to avoid data race.
- wrap `Session::Write(text)` function by strand to avoid data race.

**Description:**  
Let client write some data to server in thread A while in thread B this client recieve message, read it, and send a reply from the `async_read_some` complection handler! => possible 2 `async_write` are executed concurrently.

### Solution

For now solved using strand: the write function is wrapped in strand so that neither buffer/isWriting variables neither `async_write_some` operation aren't used\executed concurrently.

**Cons:**

- The advantage of the multiply threads somewhat gone.
With such design only `async_read` operation can be run parallel with `async_write` operation but their complection handlers can't! It's because there will be data race around `std::ostream` object if 2 completion handlers will be executed in one time. So all handlers are forced to go through one `boost::asio::strand`.

- Also I wish that there `Session` or `Client` `Write()` function can be called from several threads at one point of time
and if the writing is going on it will just push to buffer text to be sent immediately **without queuing and going through strand one-by-one**.

- I think I need at least to use other `strand` for reading operation as it can be used concurrently with writing operation. And may be `mutex` around `std::cout`.  

**Pros:**

- Very easy to implement.
