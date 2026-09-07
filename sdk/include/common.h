#pragma once
#include<string>
#include<vector>
#include<map>
#include<ctime>
namespace ai_chat_sdk
{
    //模型消息结构
     struct Message
    {
        std::string _message_id;//消息ID
        std::string _role;//角色
        std::string _content;//消息内容
        time_t _create_time;//消息发送时间
       // Message(const std::string& role,const std::string& content):_role(role),_content(content){}//初始化消息
    };

    //模型的公共配置信息
    struct ModelConfig
    {
        std::string ModelName;//模型名称
        double Temperature;//温度值
        int Max_takens;//最大token数
        virtual ~ModelConfig()=default;//默认析构函数
    };

    //模型接入方式
    //通过API接入：需要配置模型Api_key
    struct APIConfig:public ModelConfig
    {
        std::string Api_key;//模型Api_key
    };
    //通过ollama本地的方式接入
    struct ollamaConfig:public ModelConfig
    {
        std::string ModelName;//模型名称
        std::string ModelDesc;//模型描述
        std::string base_url;//ollama本地基础URL
    };



    //会话信息
    struct Session
    {
        std::string session_id;//会话ID
        std::string ModelName;//会话模型名称
        std::vector<Message> messages;//会话消息列表
        time_t create_time;//会话创建时间
        time_t update_time;//会话修改时间
        Session(const std::string& model_name=""):ModelName(model_name){}//初始化会话信息
    
    };
    //模型信息
    struct ModelInfo
    {
        std::string ModelName;//模型名称
        std::string ModelDesc;//模型描述
        std::string ModelProvider;//模型提供者
        std::string base_url;//模型基础URL
        bool is_Available;//模型是否可用
    };
}