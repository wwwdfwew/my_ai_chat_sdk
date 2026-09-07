#pragma once
#include<vector>
#include<string>
#include<map>
#include"common.h"
#include<functional>
namespace ai_chat_sdk 
{
    class LLMProvider
    {
        public:
            virtual bool ModelInit(std::map<std::string,std::string> config)=0;//模型初始化,配置模型参数
            virtual bool is_Available()=0;//模型是否可用
            virtual std::string ModelName()=0;//模型名称
            virtual std::string ModelDesc()=0;//模型描述信息
            virtual std::string SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams)=0;//发送消息
            virtual std::string SendMessageStream(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams,std::function<void(const std::string&,bool)> callback)=0;//以流式返回的方式发送消息
        protected:
            std::string api_key;//模型Api_key
            std::string base_url;//模型基础URL
    };
}