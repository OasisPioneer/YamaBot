#ifndef MESSAGE_PROCESSING_CENTER_HPP
#define MESSAGE_PROCESSING_CENTER_HPP

#include <string>

// 消息处理中心：封装与 Telegram API 的交互逻辑
class MessageProcessingCenter
{
public:
    // 构造函数：初始化 cURL 库
    MessageProcessingCenter();

    // 主运行函数：循环获取并处理消息
    void Run(const std::string& BotToken);

    // 获取消息（更新）JSON 字符串
    std::string GetMessage(const std::string& BotToken, int Offset);

    // 向指定聊天发送文本消息
    void SendMessage(const std::string& BotToken, const std::string& ChatID, const std::string& Text);

    // 官方频道和群组 ID，用于验证用户是否加入
    const std::string RequiredChannel = "-1002677226794"; // 频道
    const std::string RequiredGroup = "-1002619461912"; // 群组

    // 检查用户是否在指定聊天（频道或群组）中
    bool IsUserInChat(const std::string& BotToken, const std::string& ChatID, const std::string& UserID);

    // 析构函数：清理 cURL 库资源
    ~MessageProcessingCenter();
};

#endif // MESSAGE_PROCESSING_CENTER_HPP