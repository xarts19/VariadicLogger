# VTLogger

C++11 logger with type safe printf-like functions that use Python's format() funtion's format string mini-language.

## Types
`typedef vl::LoggerT<vl::delegate> vl::Logger` - delegates writing of messages to LogManager's worker thread
`typedef vl::LoggerT<vl::immediate> vl::ImLogger` - writes log messages in current thread

## Tutorial
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

### Log levels (`vl::LogLevel`)

* `vl::debug`
* `vl::info`
* `vl::warning`
* `vl::error`
* `vl::critical`
* `vl::nologging`

Priority grows from top to bottom. If message's log level is greater or equal to logger's log level, it gets printed. E. g. `logger.warning() << "Some warning.";` when `logger`'s log level is `vl::info`.
N. B. `vl::nologging` is only allowed for logger's log level, but not for message's. Such loggers do not pring anything.

### Options (`vl::LogOpts`)

* `vl::usual`           default logger option, no other options are set
* `vl::noendl`          don't write `'\n'` at the end of the message
* `vl::nologlevel`      don't write message's log level before each message
* `vl::notimestamp`     don't write timestamp before each message
* `vl::noflush`         don't immediately flush output streams after each message
* `vl::nologgername`    don't write logger name before each message
* `vl::nothreadid`      don't write thread id before each message
* `vl::nospace`         don't insert spaces between arguments to `operator <<`

### Formatting for `vl::safe_sprintf()` function (moddeled after Python's `str.format()` function)
* Format string specifiec a template for resulting string, where substring of the form "{X:f}" will be
  substituted by provided arguments.
* Doubled occurences of curly braces will be replaced by single occurence, e. g. "{{" will become "{" and
  "}}" will become "}" without affecting arguments substitution (use this to insert curly braces).
* X : argument number for substitution starting from 0 for first provided argument
* f : format specifier

#### Format mini-language
    format_spec ::=  [[fill]align][sign][#][0][width][,][.precision][type]
    fill        ::=  <a character other than '{' or '}'>
    align       ::=  "<" | ">" | "=" | "^"
    sign        ::=  "+" | "-" | " "
    width       ::=  integer
    precision   ::=  integer
    type        ::=  "b" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "s"
                     | "x" | "X" | "%"

* **align**:
    Symbol  | Meaning
    --------|-----------------------------------------------------------------
    **<**   | left (default for strings)
    **>**   | right (default for numerics)
    **=**   | sign aware (sign on the left, if any, number on the right, fill
            | in the middle)
    **^**   | centered (*not impemented yet*)

* **sign**:
    Symbol  | Meaning
    --------|-----------------------------------------------------------------
    **+**   | show sign for both positive and negative numbers
    **-**   | show sign only for negative numbers
    ** **   | show sign for negative and space for positive
            | (*not implemented yet*)

* **#** is only valid for integers and only for binary, octal of hex output. It specifies that the output will be prefixed by base ('0b', '0', '0x')

* **0** enables sign-aware zero-padding for numeric types. This is equivalent to a fill character of '0' with an alignment type of '='.

* **width** specifies the minimum field width

* **,** option signals the use of a comma for a thousands separator (*not implemented yet*)

* **precision** is roughly the number of digits displayed for the number. In general format, the precision is the maximum number of digits displayed. This includes digits before and after the decimal point, but does not include the decimal point itself. Digits in a scientific exponent are not included. In fixed and scientific formats, the precision is the number of digits after the decimal point. (default is 6)

* **type**

  * general
    Symbol  | Meaning
    --------|-----------------------------------------------------------------
    **s**   | string format (this is default for strings)

  * integers
    Symbol  | Meaning
    --------|-----------------------------------------------------------------
    **b**   | binary format (*not impemented yet*)
    **d**   | decimal format (default for integers)
    **o**   | octal format
    **x**   | hex format (lower-case)
    **X**   | hex format (upper-case)

  * floating-point numbers
    Symbol  | Meaning
    --------|-----------------------------------------------------------------
    **e**   | scientific notation
    **E**   | scientific notation (upper-case)
    **f**   | fixed notation
    **F**   | fixed notation (upper-case)
    **g**   | general format (default); if the number is small enough, fixed
            | format is used; if the number gets too large, the output switches
            | over to scientific format
    **G**   | general format (upper-case)
    **%**   | percentage, multiplies the number by 100 and displays in
            | fixed ('f') format, followed by a percent sign

##TODO:

* write documentation for each function
* add cross-platform coloring of messages
* implement timestamp customization
* add switch for boolalpha

