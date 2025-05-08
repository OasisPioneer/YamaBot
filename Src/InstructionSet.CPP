#include "InstructionSet.HPP"
#include <regex>
#include <cstdio>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// CURL 回调函数：将响应写入 std::string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string Ping(std::string &Parameter)
{
    // 优先判断空字符串
    // 处理 /ping 命令
    if (Parameter.empty())
    {
        return "你要 Ping 谁？";
    }
    std::string summary;
    // 使用正则判断是否合法
    std::regex ValidPattern(R"(^[a-zA-Z0-9\.\-]+$)");
    if (!std::regex_match(Parameter, ValidPattern))
    {
        return "非法目标地址";
    }
    std::string Result;
    std::string CMD = "ping -c 1 " + Parameter + " 2>&1";
    FILE *Pipe = popen(CMD.c_str(), "r");
    if (!Pipe)
    {
        return "Ping 命令执行失败";
    }
    char Buffer[256];
    while (fgets(Buffer, sizeof(Buffer), Pipe) != nullptr)
    {
        Result += Buffer;
    }
    pclose(Pipe);

    std::istringstream iss(Result);
    std::string line;
    while (std::getline(iss, line))
    {
        if (line.find("bytes from") != std::string::npos ||  // 响应行
            line.find("packet loss") != std::string::npos || // 丢包行
            line.find("rtt ") != std::string::npos)          // RTT 行
        {
            summary += line + "\n";
        }
    }
    if (summary.empty())
        summary = "目标不可达或无返回。";
    return summary;
}

std::string Whois(std::string &Parameter)
{
    // 优先判断空字符串
    if (Parameter.empty())
    {
        return "你要 Whois 谁？";
    }

    // 使用正则判断是否合法
    std::regex ValidPattern(R"(^[a-zA-Z0-9\.\-]+$)");
    if (!std::regex_match(Parameter, ValidPattern))
    {
        return "非法目标地址";
    }

    // 构建并执行 whois 命令
    std::string Result;
    std::string CMD = "whois " + Parameter + " 2>&1";
    FILE *Pipe = popen(CMD.c_str(), "r");
    if (!Pipe)
    {
        return "Whois 命令执行失败";
    }

    // 逐行读取输出
    char Buffer[2048];
    while (fgets(Buffer, sizeof(Buffer), Pipe) != nullptr)
    {
        Result += Buffer;
    }
    pclose(Pipe);

    if (Result.empty())
    {
        return "没有返回任何 Whois 数据，可能目标不存在或被限制访问";
    }
    std::istringstream Stream(Result);
    std::string Line, Filtered;
    std::vector<std::string> Keywords = {
        "Domain Name:", "Registrar:", "Creation Date:", "Registry Expiry Date:",
        "Name Server:", "Updated Date:"};

    while (std::getline(Stream, Line))
    {
        for (const auto &Key : Keywords)
        {
            if (Line.find(Key) != std::string::npos)
            {
                Filtered += Line + "\n";
                break;
            }
        }
    }
    return Filtered.empty() ? "未找到关键信息" : Filtered;
}
std::string QueryQQ(std::string &Parameter)
{
    CURL *URL;
    CURLcode Res;
    std::string ReadBuffer;

    URL = curl_easy_init();
    if (!URL)
        return "QQ 查询失败";

    std::string URLString = "https://www.xywlapi.cc/qqcx2023?qq=" + Parameter;
    curl_easy_setopt(URL, CURLOPT_URL, URLString.c_str());
    curl_easy_setopt(URL, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(URL, CURLOPT_WRITEDATA, &ReadBuffer);

    // SSL 设置
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYHOST, 2L);

    Res = curl_easy_perform(URL);
    curl_easy_cleanup(URL);
    if (Res != CURLE_OK)
        return "查询失败，疑似接口实效，请联系管理员！";
    json Json;
    try
    {
        Json = json::parse(ReadBuffer);
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON 解析错误: " << e.what() << "\n原始响应: " << ReadBuffer << std::endl;
        return "查询失败，疑似接口返回异常数据！";
    }

    if (!Json.contains("status") || !Json["status"].is_number_integer() || Json["status"].get<int>() != 200)
        return "查询失败，接口返回状态异常！";

    if (!Json.contains("message") || Json["message"].get<std::string>() != "查询成功")
        return "查询失败，该信息不存在于数据库中！";

    // 提取有效字段
    std::string Phone = Json.value("phone", "未知");
    std::string Diqu = Json.value("phonediqu", "未知");
    std::string Qqlm = Json.value("qqlm", "无");

    std::ostringstream Oss;
    Oss << "📱手机号: " << Phone << "\n"
        << "📍归属地: " << Diqu << "\n"
        << "🔗QQ老密: " << (Qqlm != "没有找到" ? Qqlm : "未找到") << "\n";

    return Oss.str();
}