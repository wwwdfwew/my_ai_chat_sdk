#include"../include/ollamaLLMProvider.h"
#include<jsoncpp/json/json.h>
#include<httplib.h>
#include <jsoncpp/json/reader.h>
#include"../include/util/my_spdlog.h"

namespace ai_chat_sdk
{
     bool OllamaProviderLLM::ModelInit(std::map<std::string,std::string> config)
     {
        if(config.find("modelName")!=config.end()&&config.find("modelDesc")!=config.end())
        {
            modelName=config["modelName"];
            modelDesc=config["modelDesc"];
            _is_Available=true;
            return true;
        }
        _is_Available=false;
        return false;
     }
     bool OllamaProviderLLM::is_Available()
     {
        return _is_Available;
     }
     std::string OllamaProviderLLM::ModelName()
     {
        return modelName;
     }
     std::string OllamaProviderLLM::ModelDesc()
     {
        return modelDesc;
     }  
     std::string OllamaProviderLLM::SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams)
     {
        //检查模型是否可用
        if(!_is_Available)
        {
            return "模型未初始化";
        }

        //构建请求参数
        double temperature=0.7;
        int num_ctx=2048;//最大token数
        if(requestParams.find("temperature")!=requestParams.end())
        {
            temperature=std::stod(requestParams["temperature"]);
        }
        if(requestParams.find("num_ctx")!=requestParams.end())
        {
            num_ctx=std::stoi(requestParams["num_ctx"]);
        }

        //构建历史消息，用json数组存储所有历史消息
        Json::Value history(Json::arrayValue);
        for(auto& message:messages)
        {
            Json::Value messageObject;
            messageObject["role"]=message._role;
            messageObject["content"]=message._content;
            history.append(messageObject);
        }

        //构建请求体：因为temperature和num_ctx是在option这个对象里面的，因此先创建options对象
        Json::Value options;
        options["temperature"]=temperature;
        options["num_ctx"]=num_ctx;

        //构建请求体
        Json::Value requestBody;
        requestBody["model"]=modelName;
        requestBody["messages"]=history;
        requestBody["options"]=options;
        requestBody["stream"]=false;//必须声明是否开启，不然会默认开启流式

        //构建http客户端
        httplib::Client client("localhost",11434);
        client.set_connection_timeout(30.0);
        client.set_read_timeout(60.0);

        //构建请求头
        httplib::Headers headers;
        

        //序列化
        Json::StreamWriterBuilder builder;
        std::string requestBodyStr=Json::writeString(builder,requestBody);
        //发送POST请求
        auto response=client.Post("/api/chat",headers,requestBodyStr,"application/json");
        if(!response)
        {
            ERR("deepseek-r1:1.5b response error:{}",to_string(response.error()));
            return "";
        }
        INFO("deepseek-r1:1.5b response status:{}",response->status);
        if(response->status!=200)
        {
            ERR("deepseek-r1:1.5b response status error:{}",response->status);
            ERR("deepseek-r1:1.5b response body:{}",response->body);
            return "";
        }
        INFO("response bod is {}",response->body);
        //说明响应成功，反序列化
        Json::CharReaderBuilder readerBuilder;
        std::istringstream iss(response->body);
        Json::Value responseBody;
        std::string errorMag;
        bool responseSuccess=Json::parseFromStream(readerBuilder, iss, &responseBody,&errorMag);
        if(!responseSuccess)
        {
            ERR("{} model response body deserialize error:{}",modelName,errorMag);
            return "";
        }
        //解析响应体
        std::string full_content;
      if(!responseBody.empty()&&responseBody.isMember("message")&&!responseBody["message"].empty()&&responseBody["message"].isMember("content")&&!responseBody["message"]["content"].empty())
      {
            std::string _content=responseBody["message"]["content"].asString();
            Message message;
            message._content=_content;
            message._role="assistant";
            messages.push_back(message);
            full_content=_content;
            INFO("{} model response content is :{}",modelName,_content);
      }
      else
      {
        ERR("{} model response content is null");
        return "";
      }

        return full_content;
     }
     std::string OllamaProviderLLM::SendMessageStream(std::vector<Message>& messages,
                                                        std::map<std::string,std::string>& requestParams,
                                                        std::function<void(const std::string&,bool)> callback)
        {
            //检查模型是否可用
            if(!_is_Available)
            {
                ERR("{} model is not available",modelName);
                return "";
            }

            //构建请求参数
            double temperature=0.7;
            int num_ctx=2048;//最大token数
            if(requestParams.find("temperature")!=requestParams.end())
            {
                temperature=std::stod(requestParams["temperature"]);
            }
            if(requestParams.find("num_ctx")!=requestParams.end())
            {
                num_ctx=std::stoi(requestParams["num_ctx"]);
            }

            //构建历史消息
            Json::Value history(Json::arrayValue);
            for(auto& message:messages)
            {
                Json::Value messageObject;
                messageObject["role"]=message._role;
                messageObject["content"]=message._content;
                history.append(messageObject);
            }
            //构建请求体
            Json::Value options;
            options["temperature"]=temperature;
            options["num_ctx"]=num_ctx;

            Json::Value requestBody;
            requestBody["model"]=modelName;
            requestBody["messages"]=history;
            requestBody["options"]=options;
            requestBody["stream"]=true;

            //构建http客户端
            httplib::Client client("localhost",11434);
            client.set_connection_timeout(60.0);
            client.set_read_timeout(300.0);

            //构建请求头
            httplib::Headers headers={
                {"Content-Type","application/json"}
            };

            //序列化
            Json::StreamWriterBuilder builder;
            std::string requestBodyStr=Json::writeString(builder,requestBody);

            //构建请求对象
            httplib::Request request;
            request.path="/api/chat";
            request.headers=headers;
            request.body=requestBodyStr;
            request.method="POST";

            //设置流式响应相关变量
            std::string buffer;
            bool streamFinish=false;
            std::string full_content;
            bool getSuccess=false;


            //设置响应处理器
            request.response_handler=[&](const httplib::Response& response)
            {
                if(response.status==200)
                {
                    INFO("{} model response success,the status is :{}",modelName,response.status);
                    getSuccess=true;
                    return true;
                }
                ERR("{} model response fail,the reason is :{}",modelName,response.reason);
                return false;
            };

            //设置数据处理器
            request.content_receiver=[&](const char *data, size_t data_length, size_t offset, size_t total_length)
            {
                if(!getSuccess)
                {
                    return false;
                }
                buffer.append(data,data_length);
                int pos=0;
                while((pos=buffer.find("\n"))!=std::string::npos)
                {
                    std::string line=buffer.substr(0,pos);
                    buffer.erase(0,pos+1);
                    if(line.empty()||line[0]==':')
                    {
                        continue;
                    }

                    //反序列化
                    Json::Value responseBody;
                    std::istringstream iss(line);
                    std::string errorMag;
                    Json::CharReaderBuilder readerBuilder;
                    auto response=Json::parseFromStream(readerBuilder,iss,&responseBody,&errorMag);
                    if(!response)
                    {
                        ERR("{} model response body deserialize error:{}",modelName,errorMag);
                        continue;
                    }
                    //解析响应体
                    if(!responseBody.empty())
                    {
                        if(responseBody.isMember("done")&&responseBody["done"].asBool())
                        {
                           INFO("{} model response done",modelName);
                           streamFinish=true;
                           callback("",true);
                           return true;
                        }
                        if(responseBody.isMember("message")&&!responseBody["message"].empty()&&responseBody["message"].isMember("content")&&!responseBody["message"]["content"].empty())
                        {
                            std::string _content=responseBody["message"]["content"].asString();
                            full_content+=_content;
                            callback(_content,false);
                        }
                    }
                    else {
                    WARN("{} model response body is empty",modelName);
                    }
                }
                return true;
            };
            //发送消息
            auto result=client.send(request);
            if(!result)
            {
                ERR("{} model send is internet error:{}",modelName,result->reason);
                return "";
            }

            return full_content;
        }
}
