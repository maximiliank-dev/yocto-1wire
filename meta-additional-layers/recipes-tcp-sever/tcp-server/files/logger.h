#pragma once

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace logger
{

/**
 * Base Class for the logger
 */
struct LogSink
{
    virtual ~LogSink () = default;
    virtual void write (const std::string &s) = 0;
};

/**
 * Write the Log to stdout (for systemd)
 */
class LogCout : public LogSink
{
    void
    write (const std::string &s)
    {
        std::cout << s;
    }
};

/**
 * Use a file to write the log to
 */
class LogFile : public LogSink
{
  private:
    std::string logging_path;
    std::fstream fs;

  public:
    explicit LogFile (std::string filename);
    explicit LogFile (std::string logging_path, std::string filename);
    ~LogFile ();
    void
    write (const std::string &s)
    {
        if (this->fs.is_open ())
        {
            this->fs << s;
        }
        else
        {
            throw std::runtime_error ("Logger Error log file is not open");
        }
    }
};

/**
 * Logger class
 * can be used Logger.log("msg ", var0, " msg 2 ", var1);
 */
class Logger
{

  private:
    std::unique_ptr<LogSink> sink_;

  public:
    explicit Logger (std::unique_ptr<LogSink> sink) : sink_ (std::move (sink)) {}

    ~Logger () = default;

    void
    log (std::string msg)
    {
        std::time_t result = std::time (nullptr);
        std::ostringstream string_builder ("", std::ios_base::ate);
        string_builder << "Log[" << std::put_time (std::localtime (&result), "%Y-%m-%d %H:%M:%S")
                       << "]: " << msg << "\n";
        sink_->write (string_builder.str ());
    }

    std::string
    convert_to_string (const char *t)
    {
        std::string s{ t };
        return s;
    }

    std::string
    convert_to_string (char *t)
    {
        std::string s{ t };
        return s;
    }

    std::string
    convert_to_string (std::string t)
    {
        std::string s{ t };
        return s;
    }

    template <typename T>
    std::string
    convert_to_string (T t)
    {
        return std::to_string (t);
    }

    template <typename T0, typename... T>
    void
    log (std::string msg, T0 t0, T... t)
    {
        msg.append (convert_to_string (t0));
        log (msg, t...);
    }
};

}