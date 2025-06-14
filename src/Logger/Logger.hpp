// MIT License
// Copyright (c) 2024-2025 Tomáš Mark
// Thread safe logger class

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "fmt/core.h"

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  // Disable min/max macros in windows.h to avoid conflicts with std::min/max
  // cxxopts.hpp uses std::min/max
  #endif

  // Undefine Raylib functions to avoid conflicts
  #define Rectangle WindowsRectangle
  #define CloseWindow WindowsCloseWindow
  #define ShowCursor WindowsShowCursor
  #define DrawText WindowsDrawText
  #define PlaySound WindowsPlaySound
  #define PlaySoundA WindowsPlaySoundA
  #define PlaySoundW WindowsPlaySoundW
  #define LoadImage WindowsLoadImage
  #define DrawTextEx WindowsDrawTextEx

  #include <windows.h>

  // Restore Raylib functions
  #undef Rectangle
  #undef CloseWindow
  #undef ShowCursor
  #undef DrawText
  #undef PlaySound
  #undef PlaySoundA
  #undef PlaySoundW
  #undef LoadImage
  #undef DrawTextEx

#endif

// Function name macros for different compilers
#if defined(__GNUC__) || defined(__clang__)
  #define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
  #define FUNCTION_NAME __FUNCSIG__
#else
  #define FUNCTION_NAME __func__
#endif

class Logger {

private:
  std::mutex logMutex_;
  std::ostringstream messageStream_;
  std::ofstream logFile_;
  bool isSkipLine_ = false;

protected:
  Logger () = default;
  ~Logger () {
    std::lock_guard<std::mutex> lock (logMutex_);
    if (logFile_.is_open ()) {
      logFile_.close ();
    }
  }

public:
  Logger (const Logger&) = delete;
  Logger (Logger&&) = delete;
  Logger& operator= (const Logger&) = delete;
  Logger& operator= (Logger&&) = delete;
  static Logger& getInstance () {
    static Logger instance;
    return instance;
  }

public:
  static void setSkipLine (bool isSkipLine) {
    getInstance ().isSkipLine_ = isSkipLine;
  }

  static bool isSkipLine () {
    return getInstance ().isSkipLine_;
  }

public:
  enum class Level { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_CRITICAL };

private:
  Level currentLevel_ = Level::LOG_INFO;

public:
  void debug (const std::string& message, const std::string& caller = "") {
    log (Level::LOG_DEBUG, message, caller);
  }

  void info (const std::string& message, const std::string& caller = "") {
    log (Level::LOG_INFO, message, caller);
  }

  void warning (const std::string& message, const std::string& caller = "") {
    log (Level::LOG_WARNING, message, caller);
  }

  void error (const std::string& message, const std::string& caller = "") {
    log (Level::LOG_ERROR, message, caller);
  }

  void critical (const std::string& message, const std::string& caller = "") {
    log (Level::LOG_CRITICAL, message, caller);
  }

  void log (Level level, const std::string& message, const std::string& caller = "") {
    std::lock_guard<std::mutex> lock (logMutex_);
    auto now = std::chrono::system_clock::now ();
    auto now_time = std::chrono::system_clock::to_time_t (now);
    std::tm now_tm;
#ifdef _WIN32
    localtime_s (&now_tm, &now_time);
#else
    localtime_r (&now_time, &now_tm);
#endif
    // Výstup na konzoli
    if (level == Level::LOG_ERROR || level == Level::LOG_CRITICAL) {
      logToStream (std::cerr, level, message, caller, now_tm);
    } else {
      logToStream (std::cout, level, message, caller, now_tm);
    }
    // Výstup do souboru, pokud je povolen
    if (logFile_.is_open ()) {
      logFile_ << "[" << std::put_time (&now_tm, "%d-%m-%Y %H:%M:%S") << "] ";
      logFile_ << "[" << (caller.empty () ? "empty caller" : caller) << "] ";
      logFile_ << "[" << levelToString (level) << "] " << message << std::endl;
    }
  }

  template <typename... Args>
  void logFmtMessage (Level level, const std::string& format, const std::string& caller,
                      Args&&... args) {
    std::string message = fmt::format (format, std::forward<Args> (args)...);
    log (level, message, caller);
  }

public:
  bool enableFileLogging (const std::string& filename) {
    std::lock_guard<std::mutex> lock (logMutex_);
    try {
      logFile_.open (filename, std::ios::out | std::ios::app);
      return logFile_.is_open ();
    } catch (const std::ios_base::failure& e) {
      std::cerr << "Failed to open log file: " << filename << " - " << e.what () << std::endl;
      return false;
    } catch (const std::exception& e) {
      std::cerr << "Failed to open log file: " << filename << " - " << e.what () << std::endl;
      return false;
    } catch (...) {
      std::cerr << "Failed to open log file: " << filename << std::endl;
      return false;
    }
  }

  void disableFileLogging () {
    std::lock_guard<std::mutex> lock (logMutex_);
    if (logFile_.is_open ()) {
      logFile_.close ();
    }
  }

  std::string levelToString (Level level) const {
    switch (level) {
    case Level::LOG_DEBUG:
      return "DBG";
    case Level::LOG_INFO:
      return "INF";
    case Level::LOG_WARNING:
      return "WRN";
    case Level::LOG_ERROR:
      return "ERR";
    case Level::LOG_CRITICAL:
      return "CRI";
    default:
      return "INF";
    }
  }

  void resetConsoleColor () {
#ifdef _WIN32
    SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
                             FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(__EMSCRIPTEN__)
// no colors, no reset
#else
    std::cout << "\033[0m";
#endif
  }

#ifdef _WIN32
  void setConsoleColorWindows (Level level) {
    const std::map<Level, WORD> colorMap
        = { { Level::LOG_DEBUG, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY },
            { Level::LOG_INFO, FOREGROUND_GREEN | FOREGROUND_INTENSITY },
            { Level::LOG_WARNING, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY },
            { Level::LOG_ERROR, FOREGROUND_RED | FOREGROUND_INTENSITY },
            { Level::LOG_CRITICAL, FOREGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_BLUE } };
    auto it = colorMap.find (level);
    if (it != colorMap.end ()) {
      SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), it->second);
    } else {
      resetConsoleColor ();
    }
  }
#else
  void setConsoleColorUnix (Level level) {
    static const std::map<Level, const char*> colorMap = { { Level::LOG_DEBUG, "\033[34m" },
                                                           { Level::LOG_INFO, "\033[32m" },
                                                           { Level::LOG_WARNING, "\033[33m" },
                                                           { Level::LOG_ERROR, "\033[31m" },
                                                           { Level::LOG_CRITICAL, "\033[95m" } };
    auto it = colorMap.find (level);
    if (it != colorMap.end ()) {
      std::cout << it->second;
    } else {
      resetConsoleColor ();
    }
  }
#endif

  void setConsoleColor (Level level) {
#ifdef _WIN32
    setConsoleColorWindows (level);
#elif EMSCRIPTEN
      // no colors
#else
    setConsoleColorUnix (level);
#endif
  }

private:
  std::string headerName_ = "DotNameLib";
  bool includeName_ = true;
  bool includeTime_ = true;
  bool includeCaller_ = true;
  bool includeLevel_ = true;

  void logToStream (std::ostream& stream, Level level, const std::string& message,
                    const std::string& caller, const std::tm& now_tm) {
    stream << buildHeader (now_tm, caller, level) << message;
    resetConsoleColor ();
    if (isSkipLine_) {
      stream << std::endl;
    }
  }

  std::string buildHeader (const std::tm& now_tm, const std::string& caller, Level level) const {
    std::ostringstream header;
    if (includeName_) {
      header << "[" << headerName_ << "] ";
    }
    if (includeTime_) {
      header << "[" << std::put_time (&now_tm, "%d-%m-%Y %H:%M:%S") << "] ";
    }
    if (includeCaller_ && !caller.empty ()) {
      header << "[" << caller << "] ";
    }
    if (includeLevel_) {
      header << "[" << levelToString (level) << "] ";
    }
    return header.str ();
  }

public:
  // Metody pro nastavení záhlaví zůstávají stejné
  void setHeaderName (const std::string& headerName) {
    std::lock_guard<std::mutex> lock (logMutex_);
    headerName_ = headerName;
  }
  void showHeaderName (bool includeName) {
    std::lock_guard<std::mutex> lock (logMutex_);
    includeName_ = includeName;
  }
  void showHeaderTime (bool includeTime) {
    std::lock_guard<std::mutex> lock (logMutex_);
    includeTime_ = includeTime;
  }
  void showHeaderCaller (bool includeCaller) {
    std::lock_guard<std::mutex> lock (logMutex_);
    includeCaller_ = includeCaller;
  }
  void showHeaderLevel (bool includeLevel) {
    std::lock_guard<std::mutex> lock (logMutex_);
    includeLevel_ = includeLevel;
  }
  void noHeader (bool noHeader) {
    if (noHeader) {
      showHeaderName (false);
      showHeaderTime (false);
      showHeaderCaller (false);
      showHeaderLevel (false);
    } else {
      showHeaderName (true);
      showHeaderTime (true);
      showHeaderCaller (true);
      showHeaderLevel (true);
    }
  }
  void visibleHeaders (bool incName, bool incTime, bool incCaller, bool incLevel) {
    showHeaderName (incName);
    showHeaderTime (incTime);
    showHeaderCaller (incCaller);
    showHeaderLevel (incLevel);
  }

public:
  class LogStream {
  public:
    LogStream (Logger& logger, Level level, const std::string& caller)
        : logger_ (logger), level_ (level), caller_ (caller) {
    }
    ~LogStream () {
      // Na konci řetězce zavoláme logování
      logger_.log (level_, oss_.str (), caller_);
    }
    template <typename T> LogStream& operator<< (const T& value) {
      oss_ << value;
      return *this;
    }
    // Přetížení pro manipulátory (např. std::endl)
    LogStream& operator<< (std::ostream& (*manip) (std::ostream&)) {
      oss_ << manip;
      return *this;
    }

  private:
    Logger& logger_;
    Level level_;
    std::string caller_;
    std::ostringstream oss_;
  };

  // Metoda, která vrací objekt LogStream pro streamové logování
  LogStream stream (Level level, const std::string& caller = "") {
    return LogStream (*this, level, caller);
  }
}; // class Logger

// clang-format off
  #define LOG Logger::getInstance()

#ifndef NDEBUG
  #define LOG_D_STREAM Logger::getInstance().stream(Logger::Level::LOG_DEBUG, FUNCTION_NAME)
  #define LOG_D_MSG(msg) Logger::getInstance().debug(msg, FUNCTION_NAME)
  #define LOG_D_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_DEBUG, format, FUNCTION_NAME, __VA_ARGS__)
#else
  #define LOG_D_STREAM if(0) Logger::getInstance().stream(Logger::Level::LOG_DEBUG, FUNCTION_NAME)
  #define LOG_D_MSG(msg) do {} while(0)
  #define LOG_D_FMT(format, ...) do {} while(0)
#endif
  
  //#define LOG_D_STREAM Logger::getInstance().stream(Logger::Level::LOG_DEBUG, FUNCTION_NAME)
  #define LOG_I_STREAM Logger::getInstance().stream(Logger::Level::LOG_INFO, FUNCTION_NAME)
  #define LOG_W_STREAM Logger::getInstance().stream(Logger::Level::LOG_WARNING, FUNCTION_NAME)
  #define LOG_E_STREAM Logger::getInstance().stream(Logger::Level::LOG_ERROR, FUNCTION_NAME)
  #define LOG_C_STREAM Logger::getInstance().stream(Logger::Level::LOG_CRITICAL, FUNCTION_NAME)
  
  //#define LOG_D_MSG(msg) Logger::getInstance().debug(msg, FUNCTION_NAME)
  #define LOG_I_MSG(msg) Logger::getInstance().info(msg, FUNCTION_NAME)
  #define LOG_W_MSG(msg) Logger::getInstance().warning(msg, FUNCTION_NAME)
  #define LOG_E_MSG(msg) Logger::getInstance().error(msg, FUNCTION_NAME)
  #define LOG_C_MSG(msg) Logger::getInstance().critical(msg, FUNCTION_NAME)
  
  //#define LOG_D_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_DEBUG, format, FUNCTION_NAME, __VA_ARGS__)
  #define LOG_I_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_INFO, format, FUNCTION_NAME, __VA_ARGS__)
  #define LOG_W_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_WARNING, format, FUNCTION_NAME, __VA_ARGS__)
  #define LOG_E_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_ERROR, format, FUNCTION_NAME, __VA_ARGS__)
  #define LOG_C_FMT(format, ...) Logger::getInstance().logFmtMessage(Logger::Level::LOG_CRITICAL, format, FUNCTION_NAME, __VA_ARGS__)
// clang-format on

#endif // LOGGER_HPP