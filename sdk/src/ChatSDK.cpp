#include "../include/ChatSDK.h"
#include"../include/DeepseekProvider.h"
#include"../include/ChatgptProvider.h"
#include"../include/ollamaLLMProvider.h"
#include"../include/util/my_spdlog.h"
#include<memory>
#include"../include/common.h"
namespace ai_chat_sdk
{
    //注册所有的模型
    void ChatSDK::RegisterAllModels(const std::vector<std::shared_ptr<ModelConfig>>& _configs)
    {
        //遍历所有模型配置
        for(const auto& config : _configs)
        {
            //根据模型名称注册不同的提供者
            if(config->ModelName=="deepseek-chat")
            {
                //创建deepseek-chat提供者对象
               auto provider = std::make_unique<DeepseekProvider>();
                //注册deepseek-chat模型
                _llmManager.RegisterModelProvider("deepseek-chat",std::move(provider));
                INFO("deepseek model {} registered",config->ModelName);
            }
            //其他模型提供者的注册逻辑...
            else if(config->ModelName=="gpt-4o-mini")
            {
                //创建openai-chat提供者对象
               auto provider = std::make_unique<ChatgptProvider>();
                //注册openai-chat模型
                _llmManager.RegisterModelProvider("gpt-4o-mini",std::move(provider));
                INFO("gpt model {} registered",config->ModelName);
            }
            else
            {
                //ollama本地接入的模型
                //遍历configs
                for(auto& config:_configs)
                {
                    //先将config转换虫ollamaConfig
                    auto _ollamaConfig=std::dynamic_pointer_cast<ollamaConfig>(config);
                    //检测该模型是否注册过
                    if(_ollamaConfig!=nullptr)
                    {   if(!_llmManager.IsModelAvailable(_ollamaConfig->ModelName))
                         {   
                        //模型未注册，那么就注册它
                        auto provider=std::make_unique<OllamaProviderLLM>();
                        _llmManager.RegisterModelProvider(_ollamaConfig->ModelName,std::move(provider));
                        INFO("ollama model {} registered",_ollamaConfig->ModelName);
                         }
                    }
                }
            }
        }
    }

    //初始化所有的模型
    void ChatSDK::InitAllModels(const std::vector<std::shared_ptr<ModelConfig>>& _configs)
    {
        //遍历所有的模型配置
        for(const auto& configs:_configs)
        {
            //首先是云端模型
            if(auto config=std::dynamic_pointer_cast<APIConfig>(configs))
            {
                //配置信息中是否有我们的模型
                if(config->ModelName=="deepseek-chat"||config->ModelName=="gpt-4o-mini")
                {
                    //初始化模型
                    if(InitCloudModels(config->ModelName,config))
                    INFO("cloud {} model init success",config->ModelName);
                //将模型信息存储到_modelConfigs中
                _modelConfigs[config->ModelName]=config;
                }
                else
                {
                    ERR("Model {} not found",config->ModelName);
                }
            }
            else if(auto config=std::dynamic_pointer_cast<ollamaConfig>(configs))
            {
               //初始化ollama模型
               if(InitOllamaModels(config->ModelName,config))
               INFO("ollama {} model init success",config->ModelName);
               //将模型信息存储到_modelConfigs中
               _modelConfigs[config->ModelName]=config;
            }
            else
            {
                ERR("config {} not found",configs->ModelName);
            }
        }
    }


    //初始化云端模型
    bool ChatSDK::InitCloudModels(const std::string& model_name,const std::shared_ptr<APIConfig>& _config)
    {
       //检测参数是否为空
       if(model_name.empty())
       {
           ERR("InitCloudModels model_name is empty");
           return false;
       }
       if(_config==nullptr)
       {
           ERR("InitCloudModels _config is nullptr");
           return false;
       }

       //初始化模型
       std::map<std::string,std::string> pragma;
       pragma["api_key"]=_config->Api_key;
       //初始化模型
       _llmManager.InitModelProvider(model_name,pragma);
       INFO("cloud {} model init success",model_name);
       _modelConfigs[model_name]=_config;
       return true;
    }

    //初始化ollama模型
    bool ChatSDK::InitOllamaModels(const std::string& model_name,const std::shared_ptr<ollamaConfig>& _config)
    {
       //检测参数是否为空
       if(model_name.empty())
       {
           ERR("InitOllamaModels model_name is empty");
           return false;
       }
       if(_config==nullptr)
       {
           ERR("InitOllamaModels _config is nullptr");
           return false;
       }

       //初始化模型
       std::map<std::string,std::string> pragma;
       pragma["modelName"]=_config->ModelName;
       pragma["modelDesc"]=_config->ModelDesc;
       //初始化模型
       _llmManager.InitModelProvider(model_name,pragma);
       //模型可用
       _modelConfigs[model_name]=_config;
        return true;
    }

    //获取所有的可用模型
    std::vector<ModelInfo> ChatSDK::ChatSDKGetAvailableModels()
    {
        return _llmManager.GetAvailableModels();
    }
    
    //获取所有的会话id
    std::vector<std::string> ChatSDK::ChatSDKGetSessionLists()
    {
        return _sessionManage.GetAllSessionIds();
    }
    
    //初始化SDK
    void ChatSDK::ChatSDKInit(const std::vector<std::shared_ptr<ModelConfig>>& _configs)
    {
        //注册所有的模型
        RegisterAllModels(_configs);
        //初始化所有的模型
        InitAllModels(_configs);
        //设置SDK可用
        isSDKAvailable=true;
    }
    //创建会话
    std::string ChatSDK::ChatSDKCreateSession(const std::string& modelName)
    {
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return "";
        }
        auto session_id=_sessionManage.CreateSession(modelName);
        return session_id;
    }
    
    //获取指定会话信息
    std::shared_ptr<Session> ChatSDK::ChatSDKGetSession(std::string& sessionId)
    {
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return nullptr;
        }
        return _sessionManage.GetSession(sessionId);
    }
    
    //获取会话中的所有消息
    std::vector<Message> ChatSDK::ChatSDKGetSessionMessages(std::string& sessionId)
    {
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return {};
        }
        return _sessionManage.GetSessionMessages(sessionId);
    }
    
    //删除指定会话
    bool ChatSDK::ChatSDKDeleteSession(std::string& sessionId)
    {
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return false;
        }
        return _sessionManage.DeleteSession(sessionId);
    }

    //发送消息：全量返回：
    std::string ChatSDK::sendMessage(const std::string& sessionId, const std::string& message)
    {

        //检测SDK是否初始化
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return "";
        }

        //检测会话是否存在
        auto session=_sessionManage.GetSession(sessionId);
        if(session==nullptr)
        {
            ERR("Session {} not found",sessionId);
            return "";
        }

        //message是否为空
        if(message.empty())
        {
            ERR("SendMessage is empty");
            return "";
        }

        //模型名称
        auto modelName= session->ModelName;
        
        //构建历史消息
        auto messages=_sessionManage.GetSessionMessages(sessionId);
        Message newMessage;
        newMessage._role="user";
        newMessage._content=message;
        messages.push_back(newMessage);
        //更新消息发送时间
        newMessage._create_time=std::time(nullptr);
        _sessionManage.AddMessage(sessionId,newMessage);
        
        //请求参数
        std::map<std::string,std::string> requestParams;
        requestParams["Temperature"] = std::to_string(_modelConfigs[modelName]->Temperature);
        requestParams["Max_takens"] = std::to_string(_modelConfigs[modelName]->Max_takens);
      
        
        INFO("LLMManager: about to call SendMessage for {}", modelName);
        //发送消息
        auto response=_llmManager.SendMessage(modelName,messages,requestParams);

        INFO("LLMManager: SendMessage returned");
        //构建消息
        Message message_;
        message_._role="assistant";
        message_._content=response;
        //更新消息发送时间
        message_._create_time=std::time(nullptr);
       
        //将得到的消息更新到会话当中
        _sessionManage.AddMessage(sessionId,message_);
        return response;
    }
    
   
    //发送消息，流式响应
     std::string ChatSDK::sendMessageStream(const std::string& sessionId, const std::string& message, 
                                            std::function<void(const std::string&, bool)> callback)
        {
            //检测SDK是否初始化
        if(!isSDKAvailable)
        {
            ERR("SDK is not available");
            return "";
        }

        //检测会话是否存在
        auto session=_sessionManage.GetSession(sessionId);
        if(session==nullptr)
        {
            ERR("Session {} not found",sessionId);
            return "";
        }

        //message是否为空
        if(message.empty())
        {
            ERR("SendMessage is empty");
            return "";
        }

        //模型名称
        auto modelName= session->ModelName;
        
        //构建历史消息
        auto messages=_sessionManage.GetSessionMessages(sessionId);
        Message newMessage;
        newMessage._role="user";
        newMessage._content=message;
        messages.push_back(newMessage);
        //更新消息发送时间
        newMessage._create_time=std::time(nullptr);
        _sessionManage.AddMessage(sessionId,newMessage);
        
        //请求参数
        std::map<std::string,std::string> requestParams;
        requestParams["Temperature"] = std::to_string(_modelConfigs[modelName]->Temperature);
        requestParams["Max_takens"] = std::to_string(_modelConfigs[modelName]->Max_takens);
      
        
        INFO("LLMManager: about to call SendMessage for {}", modelName);
        //发送消息
        auto response=_llmManager.SendMessageStream(modelName,messages,requestParams,callback);

        INFO("LLMManager: SendMessage returned");
        //构建消息
        Message message_;
        message_._role="assistant";
        message_._content=response;
       //更新消息发送时间
        message_._create_time=std::time(nullptr);
        //将得到的消息更新到会话当中
        _sessionManage.AddMessage(sessionId,message_);
        return response;


        }

}//end ai_chat_sdk