//大模型管理类
#pragma once
#include"common.h"
#include"LLMProvider.h"
#include<map>
#include<memory>
#include<vector>
#include<string>
#include<map>
#include<functional>
namespace ai_chat_sdk
{

    class LLMManager
    {
    public:
        //注册模型提供者
        bool RegisterModelProvider(const std::string& model_name,std::unique_ptr<LLMProvider> provider);
        //初始化模型提供者
        bool InitModelProvider(const std::string& model_name,const std::map<std::string,std::string>& pragma);
        //获取可用模型
        std::vector<std::string> GetAvailableModels();
        //检测模型是否可用
        bool IsModelAvailable(const std::string& model_name);
        //发送消息：全量返回
        std::string SendMessage(const std::string& model_name, std::vector<Message>& messages, std::map<std::string,std::string>& requestParams);
        //发送消息：流式返回
        std::string SendMessageStream(const std::string& model_name, std::vector<Message>& messages, std::map<std::string,std::string>& requestParams,std::function<void(const std::string&, bool)>& callback);


    private:
        //通过模型名称来管理不同的模型提供者
        std::map<std::string,std::unique_ptr<LLMProvider>> _modelProviders;//通过智能指针来管理LLMProvider,由于LLMProvider是基类，基类的指针和引用指向子类，可以实现多态性
        //在我们的项目中，新建对话之后能够得到模型的信息，所以保存模型信息
        std::map<std::string,ModelInfo>_modelInfos;
    };
}//end ai_chat_sdk
