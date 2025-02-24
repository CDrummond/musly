/**
 * This is MINILOG, a minimal header only C++ logger system.
 *
 * Copyright (c) 2013-2014, Dominik Schnitzer <dominik@schnitzer.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *- Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 *- Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Minimal Documentation
 * Currently MINILOG only logs to STDERR, however all is in place to log
 * to files.
 *
 * Usage in your C++ project:
 *     MiniLog::current_level() = logINFO;        // set debug level to INFO
 *     MINILOG(logDEBUG) << "DEBUG Log message";  // log at debug level
 *     MINILOG(logINFO) << "INFO Log message";    // log at info level
 *
 * Yields:
 *     16:20:33.130 INFO: INFO Log message        // show only info level
 *                                                // output, since DEBUG > INFO
 */

#ifndef MINILOG_H_
#define MINILOG_H_

#include <stdio.h>
#include <sstream>
#include <string>

enum minilog_level {
    logNONE,
    logERROR,
    logWARNING,
    logINFO,
    logDEBUG,
    logTRACE,
    minilog_level_max };


// minilog_get_timestr() for Windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

#include <windows.h>

inline std::string
minilog_get_timestr()
{
    #ifdef __GNUC__
    return "";
    #else
    const int MAX_LEN = 100;
    char buffer[MAX_LEN];

    if (GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0,
            "HH':'mm':'ss", buffer, MAX_LEN) == 0) {
        return "";
    }

    char result[MAX_LEN*2] = {0};
    static ULONGLONG first = GetTickCount64();
    sprintf(result, "%s.%03lld", buffer, (LONGLONG)(GetTickCount64() - first) % 1000);

    return result;
    #endif
}

// get_timestr() for Linux, OSX
#else

#include <sys/time.h>

inline std::string
minilog_get_timestr()
{
    const int MAX_LEN = 100;
    char buffer[MAX_LEN];

    time_t t;
    time(&t);
    tm r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));

    char result[MAX_LEN*2] = {0};
    struct timeval tv;
    gettimeofday(&tv, 0);
    sprintf(result, "%s.%03ld", buffer, (long)tv.tv_usec / 1000);

    return result;
}
#endif // minilog_get_timestr()


template <typename T>
class Log
{
public:
    Log();

    virtual
    ~Log();

    std::ostringstream&
    get(
            minilog_level level);

    static minilog_level&
    current_level();

    static std::string
    to_string(
            minilog_level level);

    static minilog_level
    from_string(
            const std::string& level);

protected:
    std::ostringstream os;

private:
    Log(
            const Log&);

    Log&
    operator =(
            const Log&);
};

template <typename T>
Log<T>::Log() {
}

template <typename T>
std::ostringstream& Log<T>::get(
        minilog_level level) {
    os << minilog_get_timestr() << " " << to_string(level) << ": ";
    return os;
}

template <typename T>
Log<T>::~Log() {
    os << std::endl;
    T::write(os.str());
}

template <typename T>
minilog_level&
Log<T>::current_level() {
    static minilog_level current_level = logNONE;
    return current_level;
}

template <typename T>
std::string
Log<T>::to_string(
        minilog_level level) {
    static const char* const buffer[] = {
            "NONE",
            "ERROR",
            "WARNING",
            "INFO",
            "DEBUG",
            "TRACE"};
    return buffer[level];
}

template <typename T>
minilog_level
Log<T>::from_string(
        const std::string& level) {
    if (level == "TRACE")
        return logTRACE;
    else if (level == "DEBUG")
        return logDEBUG;
    else if (level == "INFO")
        return logINFO;
    else if (level == "WARNING")
        return logWARNING;
    else if (level == "ERROR")
        return logERROR;
    else if (level == "NONE")
        return logNONE;
    else {
        Log<T>().Get(logWARNING) << "Unknown logging level '" << level
                << "'. Using INFO level as default.";
        return logINFO;
    }
}

class FileLogger
{
public:
    static FILE*&
    get_stream();

    static void
    write(
            const std::string& msg);
};

inline FILE*&
FileLogger::get_stream() {
    static FILE* stream = stderr;
    return stream;
}

inline void
FileLogger::write(
        const std::string& msg) {

    FILE* stream = get_stream();
    if (!stream) {
        return;
    }
#ifdef _OPENMP
    #pragma omp critical (minilog)
    {
#endif
    fprintf(stream, "%s", msg.c_str());
    fflush(stream);
#ifdef _OPENMP
    }
#endif
}

class MiniLog :
        public Log<FileLogger>
{
};

#define MINILOG(level) \
    if (level > MiniLog::current_level() || !FileLogger::get_stream()) ; \
    else MiniLog().get(level)


#endif /* MINILOG_H_ */
