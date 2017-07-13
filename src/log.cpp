#include <iostream>
#include <fstream>
#include <cstdarg>
#include <uv.h>

#include "log.h"

using std::endl;
using std::ostream;
using std::ofstream;

class NullLogger : public Logger {
public:
  NullLogger()
  {
    //
  }

  virtual Logger* prefix(const char *file, int line) override
  {
    return this;
  }

  virtual ostream& stream() override
  {
    return unopened;
  }

private:
  ofstream unopened;
};

class FileLogger : public Logger {
public:
  FileLogger(const char *filename) : log_stream{filename, std::ios::out | std::ios::app}
  {
    prefix(__FILE__, __LINE__);
    log_stream << "FileLogger opened." << endl;
  }

  virtual Logger* prefix(const char *file, int line) override
  {
    log_stream << "[" << file << ":" << line << "] ";
    return this;
  }

  virtual ostream& stream() override
  {
    return log_stream;
  }

private:
  ofstream log_stream;
};

static uv_key_t current_logger_key;
static const NullLogger the_null_logger;
static uv_once_t make_key_once = UV_ONCE_INIT;

static void make_key()
{
  uv_key_create(&current_logger_key);
}

Logger* Logger::current()
{
  uv_once(&make_key_once, &make_key);

  Logger* logger = (Logger*) uv_key_get(&current_logger_key);

  if (logger == nullptr) {
    uv_key_set(&current_logger_key, (void*) &the_null_logger);
  }

  return logger;
}

static void replace_logger(const Logger *new_logger)
{
  Logger *prior = Logger::current();
  if (prior != &the_null_logger) {
    delete prior;
  }

  uv_key_set(&current_logger_key, (void*) new_logger);
}

void Logger::to_file(const char *filename)
{
  replace_logger(new FileLogger(filename));
}

void Logger::disable()
{
  replace_logger(&the_null_logger);
}