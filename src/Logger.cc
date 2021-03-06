#include <iostream>
#include <exception>

#include "marley/Error.hh"
#include "marley/Logger.hh"

const char* marley::Logger::loglevel_to_str(LogLevel lev)
{
  switch (lev) {
    case LogLevel::ERROR:
      return "[ERROR]: ";
      break;
    case LogLevel::WARNING:
      return "[WARNING]: ";
      break;
    case LogLevel::DEBUG:
      return "[DEBUG]: ";
      break;
    default:
      return "";
      break;
  }
}

marley::Logger::OutStream::OutStream(std::shared_ptr<std::ostream> os,
  LogLevel lev, bool enable) : stream_(os), level_(lev), enabled_(enable),
  previously_used_(false)
{
  constexpr char error_1[] = "std::shared_ptr to ";
  constexpr char error_2[] = " passed to constructor of marley::Logger"
    "::OutStream. Please use the alternate constructor (which takes a"
    " std::ostream&) instead.";
  if (stream_.get() == &std::cout) throw marley::Error(error_1
    + std::string("std::cout") + error_2);
  else if (stream_.get() == &std::cerr) throw marley::Error(error_1
    + std::string("std::cerr") + error_2);
}

marley::Logger::OutStream::OutStream(std::ostream& os,
  LogLevel lev, bool enable) : level_(lev), enabled_(enable),
  previously_used_(false)
{
  // Avoid any deletion problems when the std::shared_ptr goes out of scope by
  // providing a custom deleter that does nothing.
  stream_ = std::shared_ptr<std::ostream>(&os, [](std::ostream*) -> void {});
}


marley::Logger::OutStreamVector&
  marley::Logger::OutStreamVector::operator<<(std::ostream&
  (*manip)(std::ostream&))
{
  for (auto s : *this) if (s.enabled_ && s.stream_) manip(*s.stream_);
  return *this;
}

marley::Logger::OutStreamVector&
  marley::Logger::OutStreamVector::operator<<(std::ios_base&
  (*manip)(std::ios_base&))
{
  for (auto s : *this) if (s.enabled_ && s.stream_) manip(*s.stream_);
  return *this;
}

void marley::Logger::flush() {
  for (auto s : streams_) if (s.stream_) s.stream_->flush();
}

void marley::Logger::newline() {
  for (auto s : streams_) if (s.stream_) (*s.stream_) << '\n';
}

void marley::Logger::clear_streams() {
  // Preserve the "previously used" state of the std::cerr stream after
  // clearing the streams
  auto cerr_stream = get_stream(&std::cerr);
  bool cerr_previously_used = false;
  if (cerr_stream && cerr_stream->previously_used_)
    cerr_previously_used = true;

  // Empty the vector of output streams for the Logger
  streams_.clear();

  // Unless the user changes the stderr logging level via a call to
  // add_stream(), the Logger will always write warning and error messages to
  // stderr.
  add_stream(std::cerr, LogLevel::WARNING);
  if (cerr_previously_used) get_stream(&std::cerr)->previously_used_ = true;
}

marley::Logger::Logger(bool log_enabled) : enabled_(log_enabled),
  old_level_(LogLevel::INFO)
{
  // Unless the user changes the stderr logging level via a call to
  // add_stream(), the Logger will always write warning and error messages to
  // stderr.
  add_stream(std::cerr, LogLevel::WARNING);
}

marley::Logger& marley::Logger::Instance() {
  static Logger instance;
  return instance;
}

bool marley::Logger::has_stream(const std::ostream& os) const {
  auto stream = get_stream(&os);
  if (stream) return true;
  // A nullptr was returned, so the stream couldn't be found
  else return false;
}

const marley::Logger::OutStream* marley::Logger::get_stream(
  const std::ostream* os) const
{
  auto end = streams_.end();
  auto iter = std::find_if(streams_.begin(), end, [os](const OutStream& s)
    -> bool { return s.stream_.get() == os; });
  if ( iter == end) return nullptr;
  else return &(*iter);
}

marley::Logger::OutStream* marley::Logger::get_stream(
  const std::ostream* os)
{
  auto end = streams_.end();
  auto iter = std::find_if(streams_.begin(), end, [os](const OutStream& s)
    -> bool { return s.stream_.get() == os; });
  if ( iter == end) return nullptr;
  else return &(*iter);
}

marley::Logger::OutStream* marley::Logger::find_stream(const std::ostream* os,
  bool& stream_enabled, LogLevel level)
{
  stream_enabled = (enabled_ && (level >= old_level_));

  auto stream = get_stream(os);
  if (stream) {
    // The stream was previously added, so update its logging level and whether
    // it is enabled.
    stream->level_ = level;
    stream->enabled_ = stream_enabled;
  }
  // Return a pointer to the OutStream object, or nullptr if it couldn't be
  // found
  return stream;
}

void marley::Logger::add_stream(std::shared_ptr<std::ostream> stream,
  LogLevel level)
{
  // Check to see whether we have already added this stream to the logger
  bool enabled;
  marley::Logger::OutStream* os = find_stream(stream.get(), enabled, level);

  if (!os) streams_.emplace_back(stream, level, enabled);
}

void marley::Logger::add_stream(std::ostream& stream, LogLevel level)
{
  // Check to see whether we have already added this stream to the logger
  bool enabled;
  marley::Logger::OutStream* os = find_stream(&stream, enabled, level);

  if (!os) streams_.emplace_back(stream, level, enabled);
}

void marley::Logger::enable(bool log_enabled) {
  enabled_ = log_enabled;
  for (auto& s : streams_) s.enabled_ = enabled_ && (s.level_ >= old_level_);
}

marley::Logger::OutStreamVector& marley::Logger::log(LogLevel lev)
{
  if (lev == LogLevel::DISABLED) throw marley::Error("marley::Logger::log()"
    " may not be called for the DISABLED logging level.");

  bool level_changed = (lev != old_level_);

  if (enabled_) {
    // Update the state of each stream based on the logging level (if
    // needed) and start a new line on each enabled stream for the new
    // message.
    for(auto& s : streams_) {
      if (level_changed) {
        s.enabled_ = (s.level_ >= lev);
        // Because writing error and warning messages to stdout will cause
        // duplication in a terminal when we're also writing to stderr, prevent
        // stdout from receiving any logger messages that are at the WARNING
        // log level or below. The user may suppress warnings/errors entirely
        // by adjusting the log level for std::cerr.
        if (s.stream_.get() == &std::cout)
          s.enabled_ = (s.enabled_) && (lev > LogLevel::WARNING);
      }
      if (s.enabled_) {
        // Insert a new line at the beginning of each logging message
        // unless this is the first message. If the stream in question is
        // std::cout and std::cerr has been previously used, also insert
        // a new line. Do the same for std::cerr.
        if (s.previously_used_) *s.stream_ << '\n';
        else {
          s.previously_used_ = true;
          if (s.stream_.get() == &std::cout) {
            auto cerr_stream = get_stream(&std::cerr);
            if (cerr_stream && cerr_stream->previously_used_)
              *s.stream_ << '\n';
          }
          if (s.stream_.get() == &std::cerr) {
            auto cout_stream = get_stream(&std::cout);
            if (cout_stream && cout_stream->previously_used_)
              *s.stream_ << '\n';
          }
        }
        *s.stream_ << loglevel_to_str(lev);
      }
    }
    // Update the old logging level
    old_level_ = lev;
  }
  return streams_;
}
