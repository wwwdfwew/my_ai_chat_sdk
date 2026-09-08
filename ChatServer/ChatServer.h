#pragma once
#include <httplib.h>
#include <memory>
#include <string>
#include<ai_chat_sdk/ChatSDK.h>
#include<atomic>

namespace ai_chat_server
{
//需要给服务器的配置信息
struct ChatServerConfig
{
    //服务器的地址和端口号
    std::string host="0.0.0.0";
    int port=8080;

    //模型的相关配置信息
    //云端模型的配置
    //deepseek-chat模型的配置
    std::string deepseekModelName;
    std::string deepseekApiKey;
    //chatgpt模型的配置
    std::string gpt4oMiniModelName;
    std::string gpt4oMiniApiKey;
    //ollama本地模型配置
    std::string ollamaModelName;
    std::string ollamaModelDesc;
    std::string ollamaBaseUrl;

    //温度值和最大token数
    float temperature=0.7;
    int max_takens=2048;
};

class ChatServer
{
public:
    //构造函数并且初始化
    ChatServer(const ChatServerConfig& config);
    //启动服务器
    bool Start();
    //停止服务器
    void Stop();
    //判断服务器是否正在运行
    bool IsRunning() const;

public:
    //处理响应错误信息
    std::string HandleErrorResponse(const std::string& message);
    //获取会话列表
     void handleGetSessionListsRequest(const httplib::Request& request, httplib::Response& response);
     //创建会话
     void handleCreateSessionRequest(const httplib::Request& request, httplib::Response& response);
     //获取可用模型
     void handleGetAvailableModelsRequest(const httplib::Request& request, httplib::Response& response);
     //获取历史消息
     void handleGetHistoryMessagesRequest(const httplib::Request& request, httplib::Response& response);
     //发送消息，全量返回
     void handleSendMessageRequest(const httplib::Request& request, httplib::Response& response);
     //发送消息，流式返回
     void handleSendMessageStreamRequest(const httplib::Request& request, httplib::Response& response);
     //删除会话
     void handleDeleteSessionRequest(const httplib::Request& request, httplib::Response& response);

     //设置路由
     void setHttpRoutes();

private:
    //服务器的配置信息
    ChatServerConfig config;
    //服务器对象
    std::unique_ptr<httplib::Server> server_;
    //ChatSDK对象
    std::shared_ptr<ai_chat_sdk::ChatSDK> sdk;
    //服务器是否正在运行，同时为了线程安全，使用原子变量
    std::atomic<bool> is_running_={false};
};
}//end ai_chat_server