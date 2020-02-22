
#include"Logging.h"
#include"AsyncLogging.h"
#include<time.h>
#include<time.h>

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;


off_t rollSize_ =500*1000*1000;
std::string Logger::logFileName_ = "./log_";

void once_init()
{
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName(),rollSize_);
    AsyncLogger_->start();
}

int g_total=0;//for test

void output(const char* msg, int len)
{
    pthread_once(&once_control_, once_init);
    AsyncLogger_->append(msg, len);
    g_total +=len;//for test
}


Logger::Impl::Impl(const char* fileName,int line)
    :stream_(),
     line_(line),
     basename_(fileName)
    {
        formatTime();
    }

void Logger::Impl::formatTime()
{
    time_t now=0;
    char timebuf[32];
    struct tm tm;
    now = time(NULL);
    gmtime_r(&now, &tm);

    strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S:\n", &tm);

    stream_ << timebuf;
}

Logger::Logger(const char *fileName, int line)
  : impl_(fileName, line)
{ }

Logger::~Logger()
{
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    const LogStream::LogBuffer& buf(stream().buffer());
    output(buf.data(), buf.length());
}
