#pragma once
#include "../include/LLMManager.h"
#include"../include/util/my_spdlog.h"
namespace ai_chat_sdk
{
    bool LLMManager::RegisterModelProvider(const std::string& model_name,std::unique_ptr<LLMProvider> provider)
    {
        //检测要注册的模型提供者是否为空
        if(provider==nullptr)
        {
            ERR("the {} model provider is nullptr",model_name);
            return false;
        }
        _modelProviders[model_name]=std::move(provider);
        _modelInfos[model_name].ModelName=model_name;
        return true;
    }
    bool LLMManager::InitModelProvider(const std::string& model_name,const std::map<std::string,std::string>& pragma)
    {
       //检测模型是否注册
       if(_modelProviders.find(model_name)==_modelProviders.end())
       {
           ERR("{} Model provider not found",model_name);
           return false;
       }
       //初始化模型
       _modelProviders[model_name]->ModelInit(pragma);
      //初始化模型之后，可以通过模型对象拿到模型的描述信息
      _modelInfos[model_name].ModelDesc=_modelProviders[model_name]->ModelDesc();
      //模型可用
      _modelInfos[model_name].is_Available = _modelProviders[model_name]->is_Available();
      INFO("LLMManager: {} is available: {}",model_name,_modelInfos[model_name].is_Available);
      return true;
    }
    //获取所有的可用模型
    std::vector<std::string> LLMManager::GetAvailableModels()
    {
        std::vector<std::string> models;
        for(auto& it:_modelInfos)
        {
            if(it.second.is_Available)
            {
                models.push_back(it.first);
            }
        }
        return models;
    }
    //检测模型是否可用
    bool LLMManager::IsModelAvailable(const std::string& model_name)
    {
        return _modelInfos[model_name].is_Available;    
    }
    //发送消息
    std::string LLMManager::SendMessage(const std::string& model_name,std::vector<Message>& messages,std::map<std::string,std::string>& requestParams)
    {
       //检查模型是否已经注册
       if(_modelProviders.find(model_name)==_modelProviders.end())
       {
           ERR("Model provider not found:{}",model_name);
           return "";
       }

       
       //检查模型是否可用
       if(!_modelInfos[model_name].is_Available)
       {
           ERR("Model provider not available:{}",model_name);
           return "";
       }



       //发送消息
       auto response=_modelProviders[model_name]->SendMessage(messages,requestParams);
       return response;
    }

    //发送消息：流式返回
    std::string LLMManager::SendMessageStream(const std::string& model_name, std::vector<Message>& messages, std::map<std::string,std::string>& requestParams,std::function<void(const std::string&, bool)>& callback)
    {
        //检查模型是否已经注册
        if(_modelProviders.find(model_name)==_modelProviders.end())
        {
            ERR("Model provider not found:{}",model_name);
            return "";
        }
        //检查模型是否可用
        if(!_modelInfos[model_name].is_Available)
        {
            ERR("Model provider not available:{}",model_name);
            return "";
        }
        //发送消息
        auto response=_modelProviders[model_name]->SendMessageStream(messages,requestParams,callback);
        return response;
    }




}//end ai_chat_sdk