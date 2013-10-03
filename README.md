VTLogger
========

C++11 logger with type safe printf-like functions that use Python's format() funtion's format string mini-language.

Types
-----
`typedef vl::LoggerT<vl::delegate> vl::Logger` - delegates writing of messages to LogManager's worker thread
`typedef vl::LoggerT<vl::immediate> vl::ImLogger` - writes log messages in current thread

Tutorial
--------
If you want to use `vl::Logger` class or `vl::get_logger()` and `vl::set_logger()` functions, you need to create `vl::LogManager` that will exist for the duration of any output `vl::Logger` operations or `vl::get_logger()` and `vl::set_logger()` calls. Easiest way to do this is to create an instance of `vl::LogManager` on the stack at the begging of `main()`.

    vl::LogManager log_manager;

It will be destroyed cleanly at the end of main. Beware, however, that in this case you can't use logging in constructors or destructors of static objects, because they run either before or after `main()`.
`vl::set_logger()` function stores the logger in `vl::LogManager`'s interal map. If a logger with the same name is already stored there, it is overwritten. `vl::get_logger()` retrieves the logger by name. Changes to retrieved instances of loggers do not propagate to other instances, retreived in another place. This functions are thread-safe.
`vl::LogManager` also provides a thread for writing log messages that `vl::Logger` uses. Thread is created in `vl::LogManager`'s constructor and joined in it's destructor. Destructor block until all messages have been writted.

Creating and using a logger:

    vl::Logger logger("default");
    logger.set_cout();
    logger.add_stream("logfile.log");

    logger.log(debug, "{0} {1}", "Hello", "world!");
    logger.debug() << "Hello" << "world";

Changing options:

    logger.set(vl::noendl);
    logger.set(vl::nospace);
    logger.unset(vl::nospace);
    logger.reset();

Log levels (`vl::LogLevel`)
---------------------------

* `vl::debug`
* `vl::info`
* `vl::warning`
* `vl::error`
* `vl::critical`
* `vl::nologging`

Priority grows from top to bottom. If message's log level is greater or equal to logger's log level, it gets printed. E. g. `logger.warning() << "Some warning.";` when `logger`'s log level is `vl::info`.
N. B. `vl::nologging` is only allowed for logger's log level, but not for message's. Such loggers do not pring anything.

Options (`vl::LogOpts`)
-----------------------

* `vl::usual`           default logger option, no other options are set
* `vl::noendl`          don't write `'\n'` at the end of the message
* `vl::nologlevel`      don't write message's log level before each message
* `vl::notimestamp`     don't write timestamp before each message
* `vl::noflush`         don't immediately flush output streams after each message
* `vl::nologgername`    don't write logger name before each message
* `vl::nothreadid`      don't write thread id before each message
* `vl::nospace`         don't insert spaces between arguments to `operator <<`


TODO:
-----

* write documentation for each function
* add cross-platform coloring of messages
* implement timestamp customization

