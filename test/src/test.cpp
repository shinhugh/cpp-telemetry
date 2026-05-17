#include "telemetry/living_span.h"
#include "telemetry/log.h"
#include "telemetry/log_severity.h"
#include "telemetry/log_value.h"
#include "telemetry/span.h"
#include "telemetry/trace.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------

struct Data
{
  int i;
  std::string s;
};

// -----------------------------------------------------------------------------

static void WriteSpanToStdout(
    const telemetry::Span &);

static void WriteLogToStdout(
    const telemetry::Span &, std::chrono::system_clock::time_point,
    telemetry::LogSeverity, const std::string &event,
    const std::vector<std::pair<std::string, telemetry::LogValue>> &fields);

static void SerializeLogValue(
    std::ostream &, const telemetry::LogValue &);

static void TestTracing();

static void TestLogging();

static telemetry::LogValue Capture(
    const Data &);

static telemetry::LogValue Capture(
    Data &&);

// -----------------------------------------------------------------------------

int main()
{
  telemetry::RegisterSpanHandler(WriteSpanToStdout);
  telemetry::RegisterLogHandler(WriteLogToStdout);

  TestTracing();

  std::cout << std::endl;

  TestLogging();

  return 0;
}

// -----------------------------------------------------------------------------

static void WriteSpanToStdout(
    const telemetry::Span &span)
{
  std::stringstream ss;
  ss << "span:" << std::endl
     << "  trace ID:       " << span.m_traceId << std::endl
     << "  span ID:        " << span.m_spanId << std::endl
     << "  parent span ID: "
     << (span.m_parentSpanId ? *span.m_parentSpanId : "<none>") << std::endl;
  std::cout << ss.str();
}

// -----------------------------------------------------------------------------

static void WriteLogToStdout(
    const telemetry::Span &span, std::chrono::system_clock::time_point time,
    telemetry::LogSeverity severity, const std::string &event,
    const std::vector<std::pair<std::string, telemetry::LogValue>> &fields)
{
  std::time_t tt = std::chrono::system_clock::to_time_t(time);
  std::tm localtime = *std::localtime(&tt);

  std::ostringstream os;
  os << std::left << '[' << std::put_time(&localtime, "%F %T") << ']'
     << std::endl
     << '[' << span.m_traceId << "] [" << span.m_spanId << ']' << std::endl
     << '<' << std::setw(6);
  switch (severity)
  {
    case telemetry::LogSeverity::DEBUG:
      os << "DEBUG>";
      break;
    case telemetry::LogSeverity::INFO:
      os << "INFO>";
      break;
    case telemetry::LogSeverity::WARN:
      os << "WARN>";
      break;
    case telemetry::LogSeverity::ERROR:
      os << "ERROR>";
      break;
    case telemetry::LogSeverity::FATAL:
      os << "FATAL>";
      break;
  }
  os << ' ' << event << std::endl;
  if (!fields.empty())
  {
    os << '{';
    bool addComma = false;
    for (const auto &[k, v] : fields)
    {
      if (addComma)
      {
        os << ',';
      }
      os << ' ' << k << ": ";
      SerializeLogValue(os, v);
      addComma = true;
    }
    os << " }" << std::endl;
  }

  std::cout << os.str();
}

// -----------------------------------------------------------------------------

static void SerializeLogValue(
    std::ostream &os, const telemetry::LogValue &v)
{
  bool addComma = false;
  switch (v.GetType())
  {
    case telemetry::LogValue::Type::VOID:
      os << "<none>";
      break;
    case telemetry::LogValue::Type::BOOLEAN:
      os << (v.GetBoolean() ? "true" : "false");
      break;
    case telemetry::LogValue::Type::INTEGER:
      os << v.GetInteger();
      break;
    case telemetry::LogValue::Type::FLOAT:
      os << v.GetFloat();
      break;
    case telemetry::LogValue::Type::STRING:
      os << v.GetString();
      break;
    case telemetry::LogValue::Type::ARRAY:
      os << '[';
      for (const telemetry::LogValue &value : v.GetArray())
      {
        if (addComma)
        {
          os << ',';
        }
        os << ' ';
        SerializeLogValue(os, value);
        addComma = true;
      }
      os << " ]";
      break;
    case telemetry::LogValue::Type::OBJECT:
      os << '{';
      for (const auto &[key, value] : v.GetObject())
      {
        if (addComma)
        {
          os << ',';
        }
        os << ' ' << key << ": ";
        SerializeLogValue(os, value);
        addComma = true;
      }
      os << " }";
      break;
  }
}

// -----------------------------------------------------------------------------

static void TestTracing()
{
  telemetry::LivingSpan span{"origin"};

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::thread thread1{[
          parentSpan = static_cast<telemetry::Span>(span)]()
      {
        telemetry::LivingSpan span{parentSpan};

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::thread thread1{[
                parentSpan = static_cast<telemetry::Span>(span)]()
            {
              telemetry::LivingSpan span{parentSpan};

              std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }};

        std::thread thread2{[
                parentSpan = static_cast<telemetry::Span>(span)]()
            {
              telemetry::LivingSpan span{parentSpan};

              std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }};

        thread1.join();
        thread2.join();

        telemetry::FlushSpans();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }};

  std::thread thread2{[
          parentSpan = static_cast<telemetry::Span>(span)]()
      {
        telemetry::LivingSpan span{parentSpan};

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }};

  thread1.join();
  thread2.join();

  telemetry::FlushSpans();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  span.Close();

  telemetry::FlushSpans();
}

// -----------------------------------------------------------------------------

static void TestLogging()
{
  telemetry::LivingSpan span;

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "bool", telemetry::Field("value", true));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "long_long", telemetry::Field("value", 1LL));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "double", telemetry::Field("value", 1.2));

  {
    std::string data{"hello"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "string", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30)
              << "string move check: " << (data.empty() ? "pass" : "fail")
              << std::endl;
  }

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "int", telemetry::Field("value", 1));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "unsigned_int", telemetry::Field("value", 1U));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "long", telemetry::Field("value", 1L));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "unsigned_long", telemetry::Field("value", 1UL));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "float", telemetry::Field("value", 1.2F));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "cstring", telemetry::Field("value", "hello"));

  telemetry::Log(
      span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
      "address", telemetry::Field("value", static_cast<const void *>("")));

  {
    std::optional<std::string> data{"hello"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "optional", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "optional move check: "
              << (!data || data->empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::unique_ptr<std::string> data = std::make_unique<std::string>("hello");
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "unique_ptr", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "unique_ptr move check: "
              << (!data || data->empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::shared_ptr<std::string> data = std::make_shared<std::string>("hello");
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "shared_ptr", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "shared_ptr move check: "
              << (!data || data->empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::pair<std::string, int> data = std::make_pair<std::string, int>("a", 1);
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "pair", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30)
              << "pair move check: " << (data.first.empty() ? "pass" : "fail")
              << std::endl;
  }

  {
    std::tuple<std::string, int, bool> data =
        std::make_tuple<std::string, int, bool>("a", 1, true);
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "tuple", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "tuple move check: "
              << (std::get<0>(data).empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::vector<std::string> data{"ab", "cd", "ef"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "vector", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    bool moveSuccess = true;
    for (const std::string &e : data)
    {
      if (!e.empty())
      {
        moveSuccess = false;
      }
    }
    std::cout << std::left << std::setw(30)
              << "vector move check: " << (moveSuccess ? "pass" : "fail")
              << std::endl;
  }

  {
    std::unordered_set<std::string> data{"ab", "cd", "ef"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "unordered_set", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "unordered_set move check: "
              << (data.empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::set<std::string> data{"ab", "cd", "ef"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "set", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30)
              << "set move check: " << (data.empty() ? "pass" : "fail")
              << std::endl;
  }

  {
    std::unordered_map<std::string, int> data{
        {"ab", 12}, {"cd", 34}, {"ef", 56}};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "unordered_map", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30) << "unordered_map move check: "
              << (data.empty() ? "pass" : "fail") << std::endl;
  }

  {
    std::map<std::string, int> data{{"ab", 12}, {"cd", 34}, {"ef", 56}};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "map", telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));

    std::cout << std::left << std::setw(30)
              << "map move check: " << (data.empty() ? "pass" : "fail")
              << std::endl;
  }

  {
    Data data{1, "abc"};
    telemetry::Log(
        span, std::chrono::system_clock::now(), telemetry::LogSeverity::DEBUG,
        "custom_type", telemetry::Field("ptr", &data),
        telemetry::Field("ref_lvalue", data),
        telemetry::Field("ref_rvalue", std::move(data)));
  }

  std::cout << std::endl;

  telemetry::FlushLogs();
}

// -----------------------------------------------------------------------------

static telemetry::LogValue Capture(
    const Data &v)
{
  std::unordered_map<std::string, telemetry::LogValue> object{
      {"i", telemetry::Capture(v.i)}, {"s", telemetry::Capture(v.s)}};
  return telemetry::Capture(std::move(object));
}

// -----------------------------------------------------------------------------

static telemetry::LogValue Capture(
    Data &&v)
{
  std::unordered_map<std::string, telemetry::LogValue> object{
      {"i", telemetry::Capture(v.i)},
      {"s", telemetry::Capture(std::move(v.s))}};
  return telemetry::Capture(std::move(object));
}
