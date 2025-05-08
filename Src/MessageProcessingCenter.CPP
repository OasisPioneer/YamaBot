#include "MessageProcessingCenter.HPP"
#include "InstructionSet.HPP"

#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// CURL 回调函数：将响应写入 std::string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

MessageProcessingCenter::MessageProcessingCenter()
{
    // 初始化全局 CURL 环境
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void MessageProcessingCenter::Run(const std::string &BotToken)
{
    int Offset = 0;
    while (true)
    {
        // 获取更新（JSON）
        std::string ResPonse = GetMessage(BotToken, Offset);
        if (ResPonse.empty())
        {
            std::cerr << "Warning: getUpdates 返回空响应，可能网络问题或无新消息。" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        json Json;
        try
        {
            Json = json::parse(ResPonse);
        }
        catch (const std::exception &e)
        {
            std::cerr << "JSON 解析错误: " << e.what() << "\n原始响应: " << ResPonse << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (!Json.contains("ok") || !Json["ok"].is_boolean() || !Json["ok"].get<bool>())
        {
            std::cerr << "Error: getUpdates 请求失败，响应: " << Json.dump() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (!Json.contains("result") || !Json["result"].is_array())
        {
            // 无有效结果
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        // 遍历所有更新
        for (auto &Message : Json["result"])
        {
            // 更新 Offset 以避免重复获取
            if (Message.contains("update_id") && Message["update_id"].is_number_integer())
            {
                // Offset = std::max(Offset, Message["update_id"].get<int>() + 1);
                int id = Message["update_id"].get<int>();
                Offset = std::max(Offset, id + 1);
            }

            // 处理用户消息
            if (Message.contains("message") && Message["message"].is_object())
            {
                const auto &MessageContent = Message["message"];

                // 获取用户ID
                if (!MessageContent.contains("from") || !MessageContent["from"].contains("id"))
                {
                    std::cerr << "Error: " << MessageContent["from"].contains("id") << std::endl;
                    continue; // 无效ID格式，跳过
                }

                std::string UserID = std::to_string(MessageContent["from"]["id"].get<int>());
                std::string ChatID = std::to_string(MessageContent["chat"]["id"].get<int>());

                bool InChannel = IsUserInChat(BotToken, RequiredChannel, UserID);
                bool InGroup = IsUserInChat(BotToken, RequiredGroup, UserID);

                if (!InChannel || !InGroup)
                {
                    // 用户未加入频道或群组
                    
                    // 用户未同时加入频道和群组，提醒并跳过
                    if (MessageContent.contains("chat") && MessageContent["chat"].contains("id"))
                    {
                        SendMessage(BotToken, ChatID, "必须加入官方频道和群组才能使用本机器人\n https://t.me/OasisPioneerChannel\n https://t.me/OasisPioneerGroup");
                        std::cout << "用户 " << UserID << " 未加入频道或群组" << std::endl;
                    }
                    continue;
                }

                if (!MessageContent.contains("chat") || !MessageContent["chat"].contains("id"))
                {
                    continue;
                }
                if (!MessageContent.contains("text") || !MessageContent["text"].is_string())
                {
                    std::cout << "用户: " << UserID << " 发送了: " << MessageContent["text"].get<std::string>() << std::endl;
                    continue;
                }

                std::string Text = MessageContent["text"].get<std::string>();
                std::istringstream iss(Text);
                std::string Command, Parameter;
                size_t SpacePos = Text.find(' ');
                if (SpacePos != std::string::npos)
                {
                    Command = Text.substr(0, SpacePos);
                    Parameter = Text.substr(SpacePos + 1);
                }
                else
                {
                    Command = Text;
                }

                std::cout << "用户: " << ChatID << " 发送了: " << Text << std::endl;

                if (Command == "/start")
                {
                    // 处理 /start 命令
                    SendMessage(BotToken, ChatID, "\n 这里是你爹古董店，有事跟爹说！\n 不知道怎么跟爹说话就发 /help 让你爹好好教你。");
                }
                else if (Command == "/Ping")
                {
                    SendMessage(BotToken, ChatID, Ping(Parameter));
                }
                else if (Command == "/Whois")
                {
                    SendMessage(BotToken, ChatID, Whois(Parameter));
                }
                else if (Command == "/QQ")
                {
                    SendMessage(BotToken, ChatID, QueryQQ(Parameter));
                }
                else if (Command == "/Query")
                {
                    SendMessage(BotToken, ChatID, "你要查询啥？");
                }
                else if (Command == "/Test")
                {
                    SendMessage(BotToken, ChatID, "测试成功！");
                }
                else if (Command == "/Help")
                {
                    SendMessage(BotToken, ChatID, "可用命令:\n/Start - 启动机器人\n/Ping - 测试网络延迟\n/Whois - 查询域名信息\n/QQ - 查询 QQ 号码信息\n/Test - 测试命令\n/Help - 获取帮助信息");
                } else {
                    // 处理其他命令
                    SendMessage(BotToken, ChatID, "你瞎发你大爸，不知道咋跟你爹说话就发 /Help 让你爹好好教教你。");
                }
                // Echo the message back
                // SendMessage(BotToken, ChatID, Text);
            }
        }

        // std::this_thread::sleep_for(std::chrono::seconds(1)); // 延时防止频繁请求
    }
}

std::string MessageProcessingCenter::GetMessage(const std::string &BotToken, int Offset)
{
    CURLcode Res;
    std::string ReadBuffer;

    if (CURL *URL = curl_easy_init())
    {
        // 构造请求 URL
        std::string URLString = "https://api.telegram.org/bot" + BotToken + "/getUpdates?timeout=5&offset=" + std::to_string(Offset);
        curl_easy_setopt(URL, CURLOPT_URL, URLString.c_str());
        curl_easy_setopt(URL, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(URL, CURLOPT_WRITEDATA, &ReadBuffer);

        // curl_easy_setopt(URL, CURLOPT_VERBOSE, 1L);
        // SSL 设置
        curl_easy_setopt(URL, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(URL, CURLOPT_SSL_VERIFYHOST, 2L);

        Res = curl_easy_perform(URL);
        if (Res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(Res) << std::endl;
        }
        curl_easy_cleanup(URL);
    }
    return ReadBuffer;
}

void MessageProcessingCenter::SendMessage(const std::string &BotToken, const std::string &ChatID, const std::string &Text)
{
    using json = nlohmann::json;
    json Json = {
        {"chat_id", ChatID},
        {"text", Text}};
    std::string JsonString = Json.dump();

    CURL *URL;
    CURLcode Res;
    std::string ReadBuffer;

    URL = curl_easy_init();
    if (!URL)
        return;
    struct curl_slist *Headers = NULL;
    Headers = curl_slist_append(Headers, "Content-Type: application/json");

    std::string URLString = "https://api.telegram.org/bot" + BotToken + "/sendMessage";
    curl_easy_setopt(URL, CURLOPT_URL, URLString.c_str());
    curl_easy_setopt(URL, CURLOPT_HTTPHEADER, Headers);
    curl_easy_setopt(URL, CURLOPT_POSTFIELDS, JsonString.c_str());
    curl_easy_setopt(URL, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(URL, CURLOPT_WRITEDATA, &ReadBuffer);

    // curl_easy_setopt(URL, CURLOPT_VERBOSE, 1L);
    // SSL 设置
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYHOST, 2L);

    if (Res = curl_easy_perform(URL); Res != CURLE_OK)
    {
        std::cerr << "curl sendMessage 请求失败: " << curl_easy_strerror(Res) << std::endl;
    }

    curl_easy_cleanup(URL);
    curl_slist_free_all(Headers);
}

bool MessageProcessingCenter::IsUserInChat(const std::string &BotToken, const std::string &ChatID, const std::string &UserID)
{
    CURL *URL;
    CURLcode Res;
    std::string ReadBuffer;

    URL = curl_easy_init();
    if (!URL)
        return false;

    std::string URLString = "https://api.telegram.org/bot" + BotToken + "/getChatMember?chat_id=" + ChatID + "&user_id=" + UserID;

    curl_easy_setopt(URL, CURLOPT_URL, URLString.c_str());
    curl_easy_setopt(URL, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(URL, CURLOPT_WRITEDATA, &ReadBuffer);

    // curl_easy_setopt(URL, CURLOPT_VERBOSE, 1L);
    // SSL 设置
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(URL, CURLOPT_SSL_VERIFYHOST, 2L);

    Res = curl_easy_perform(URL);
    curl_easy_cleanup(URL);
    if (Res != CURLE_OK)
        return false;

    json Json;
    try
    {
        Json = json::parse(ReadBuffer);
    }
    catch (const std::exception &e)
    {
        std::cerr << "getChatMember JSON 解析错误: " << e.what() << "\n原始响应: " << ReadBuffer << std::endl;
        return false;
    }

    if (!Json.contains("ok") || !Json["ok"].is_boolean() || !Json["ok"].get<bool>() || !Json.contains("result"))
    {
        std::cerr << "Error: getChatMember 返回无效响应: " << Json.dump() << std::endl;
        std::cerr << "ReadBuffer : " << ReadBuffer <<  std::endl;
        return false;
    }

    const auto &Result = Json["result"];
    if (Result.contains("status") && Result["status"].is_string())
    {
        // std::string Status = Result["status"];
        std::string Status = Result["status"].get<std::string>();
        return (Status != "left" && Status != "kicked");
    }
    else
    {
        std::cerr << "Error: getChatMember 响应缺少 status 字段或类型不匹配: " << Json.dump() << std::endl;
        return false;
    }
}

MessageProcessingCenter::~MessageProcessingCenter()
{
    // 清理全局 cURL 环境
    curl_global_cleanup();
}