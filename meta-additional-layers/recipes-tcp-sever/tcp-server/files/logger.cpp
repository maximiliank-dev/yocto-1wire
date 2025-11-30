
#include "logger.h"

using namespace logger;

LogFile::LogFile (std::string filename)
{
    this->fs = std::fstream (this->logging_path + "/" + filename, std::ios::app);
}

LogFile::LogFile (std::string logging_path, std::string filename)
{
    this->fs = std::fstream (logging_path + "/" + filename, std::ios::app);
}

LogFile::~LogFile ()
{
    if (this->fs.is_open ())
    {
        this->fs.close ();
    }
}