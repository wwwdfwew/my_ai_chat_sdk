#pragma once
#include"common.h"
#include"LLMManager.h"
#include"SessionManage.h"
#include<vector>
#include<unordered_map>
#include<functional>
#include<map>
#include<string>
namespace ai_chat_sdk
{
    class ChatSDK
    {
public:
    //初始化SDK
    void ChatSDKInit(const std::vector<std::shared_ptr<ModelConfig>>& _configs);
    //检测SDK是否可用
    bool ChatSDKCheckAvailable();
    //创建会话
    std::string ChatSDKCreateSession(const std::string& modelName);
    //获取指定会话信息
    std::shared_ptr<Session> ChatSDKGetSession(std::string& sessionId);
    //获取会话中的所有消息
    std::vector<Message> ChatSDKGetSessionMessages(std::string& sessionId);
    //删除指定会话
    bool ChatSDKDeleteSession(std::string& sessionId);

    std::string sendMessage(const std::string& sessionId, const std::string& message);
        // 发送消息 - 增量返回 - 流式响应
    std::string sendMessageStream(const std::string& sessionId, const std::string& message, 
                                            std::function<void(const std::string&, bool)> callback); 

private:
    //注册所有的模型
    void RegisterAllModels(const std::vector<std::shared_ptr<ModelConfig>>& _configs);
    //初始化所有的模型
    void InitAllModels(const std::vector<std::shared_ptr<ModelConfig>>& _configs);

    //初始化云端模型
    bool InitCloudModels(const std::string& model_name,const std::shared_ptr<APIConfig>& _config);
    //初始化ollama模型
    bool InitOllamaModels(const std::string& model_name,const std::shared_ptr<ollamaConfig>& _config);
private:
   //保存用户传入的模型消息
   std::unordered_map<std::string,std::shared_ptr<ModelConfig>> _modelConfigs;
   //大模型管理
   LLMManager _llmManager;
   //会话管理和持久化存储
   SessionManage _sessionManage;
   //SDK是否可用
   bool isSDKAvailable;
        };
}//end ai_chat_sdk