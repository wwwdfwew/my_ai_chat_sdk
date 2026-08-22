#include"ChatgptProvider.h"
#include<httplib.h>
#include<jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <net/if.h>
#include"../include/util/my_spdlog.h"

namespace ai_chat_sdk
{
    //初始化模型参数
    bool ChatgptProvider::ModelInit(std::map<std::string,std::string> config)
    {
        //在传入的config中看是否能够找到Api_key
        if(config.find("api_key")!=config.end()){
        api_key=config["api_key"];
        }
        else{
        return false;
        }
       //在传入的config中看是否能够找到base_url
       if(config.find("base_url")!=config.end()){
       base_url=config["base_url"];
       }
       else{
       return false;
       }
       
       is_Available_=true;//模型初始化成功
       return true;
    }

    bool ChatgptProvider::is_Available()//模型是否可用
    {
        return is_Available_;
    }

    std::string ChatgptProvider::ModelName()//模型名称
    {
        return "gpt-4o-mini";//模型名称
    }
    std::string ChatgptProvider::ModelDesc()//模型描述信息
    {
        return "这是openai公司开发的大语言模型——gpt-4o-mini";//模型描述信息
    }

    //模型全量返回
    std::string ChatgptProvider::SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams)
    {
        //1.检查模型是否可用
        if(!is_Available())
        {
            ERR("Chatgpt model is not init");
            return "";
        }
        //2.构建请求参数
        double temperature=0.7;
        int max_output_tokens=2048;
        if(requestParams.find("Temperature")==requestParams.end()){
            temperature=std::stod(requestParams["Temperature"]);
        }
        if(requestParams.find("Max_takens")==requestParams.end()){
            max_output_tokens=std::stoi(requestParams["Max_takens"]);
        }

        //3.构建历史消息列表,将每次的message以Json::Value对象的方式存储到一个Json::Value数组中
        Json::Value HistoryMessages(Json::arrayValue);
        for(auto& message:messages)
        {
            Json::Value messageobject;
            messageobject["role"]=message._role;
            messageobject["content"]=message._content;
            HistoryMessages.append(messageobject);
        }
        //4.构建请求体
        Json::Value RequestBody;
        RequestBody["model"]=ModelName();
        RequestBody["messages"]=HistoryMessages;//数组
        RequestBody["temperature"]=temperature;
        RequestBody["max_output_tokens"]=max_output_tokens;
        //5.序列化Json对象
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::string requestBody =Json::writeString(builder,RequestBody);
        //5.构建请求头
        httplib::Headers headers={{"Authorization","Bearer "+api_key},
                                    {"Content-Type","application/json"}};
       //6.构建一个http客户端
        httplib::Client client(base_url);
        //设置连接超时时间
        client.set_connection_timeout(30);
        //设置响应超时时间
        client.set_read_timeout(60);
        
        //7.发送post请求
        auto response=client.Post("/v1/chat/completions",headers,requestBody,"application/json");
       //是否响应成功：opertor bool()重载
       if(!response){
            ERR("Chatgpt model response error");
            return "";
       }
       INFO("Chatgpt model response status is:{}",response->status);
       //响应成功，判断响应状态码是否为200
       if(response->status!=200){
            ERR("Chatgpt model response status is not 200");
            return "";
       }
       //8.解析响应体
       //反序列化
       Json::CharReaderBuilder charbuilder;
       std::string error;
       Json::Value responseJson;
       std::istringstream iss(response->body);
       bool success=Json::parseFromStream(charbuilder, iss, &responseJson,&error);//反序列化iss中的内容到responseJson
       if(!success){
            ERR("Chatgpt model response paese faile:{}",error);
            return "";
       }
       //取出content内容
      
       if(!responseJson.empty()&&
          responseJson["output"].isNull()&&
          !responseJson["output"].empty()&&
          responseJson["output"][0]["content"].isNull())
          {
            std::string content=responseJson["output"][0]["content"].asString();
            //将content添加到用户传过来的messages参数中，这样用户下次再发送消息时，就可以将content作为历史消息发送
            Message message;
            message._role="assistant";
            message._content=content;
            messages.push_back(message);
            return content;
          }
        
        ERR("Chatgpt model resonse content is fails");
        return "";
    }
    std::string ChatgptProvider::SendMessageStream(std::vector<Message>& messages,
                                    std::map<std::string,std::string>& requestParams,
                                    std::function<void(const std::string&,bool)> callback)
        {
            //1.检查模型是否可用
            if(!is_Available())
            {
                ERR("Chatgpt model is not init");
                return "";
            }
            //2.构建请求参数
            double temperature=0.7;
            int max_output_tokens=2048;
            if(requestParams.find("Temperature")==requestParams.end()){
                temperature=std::stod(requestParams["Temperature"]);
            }
            if(requestParams.find("Max_takens")==requestParams.end()){
                max_output_tokens=std::stoi(requestParams["Max_takens"]);
            }
            //3.构建历史消息列表
            Json::Value HistoryMessages(Json::arrayValue);
            for(auto& message:messages)
            {
                Json::Value messageobject;
                messageobject["role"]=message._role;
                messageobject["content"]=message._content;
                HistoryMessages.append(messageobject);
            }
            //4.构建请求体
            Json::Value RequestBody;
            RequestBody["model"]=ModelName();
            RequestBody["messages"]=HistoryMessages;//数组
            RequestBody["temperature"]=temperature;
            RequestBody["max_output_tokens"]=max_output_tokens;
            RequestBody["stream"]=true;
            //5.序列化Json对象
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::string requestBody =Json::writeString(builder,RequestBody);
            //6.构造请求对象，设置响应处理器和数据处理器
            //设置请求头
            httplib::Headers headers={{"Authorization","Bearer "+api_key},
                                    {"Content-Type","application/json"}};

            //6.构建一个http客户端
            httplib::Client client(base_url);
            //设置连接超时时间
            client.set_connection_timeout(30);
            //设置响应超时时间
            client.set_read_timeout(300);
            httplib::Request request;
            request.method="POST";
            request.path="/v1/chat/completions";
            request.body=requestBody;
            request.headers=headers;

            std::string buffer;//接收SSE数据块
            int success=0;
            
            //7.设置响应处理器
            request.response_handler=[](const httplib::Response& response){
                if(response.status!=200){
                    ERR("Chatgpt model response status is not 200");
                    ERR("Chatgpt model response error",response.reason);
                    return false;
                }
                return true;
            };

            //8.设置数据处理器
            request.content_receiver=[&](const char *data, size_t data_length, size_t offset, size_t total_length){
                if(success!=200)
                {
                    return false;
                }
                buffer.append(data,data_length);//j将接收到的数据添加到buffer中
                //由于TCP协议接收到的数据不一定是完整的，而SSE协议的格式是固定的，因此我们需要解析来对buffer中的数据进行处理
                int pos=0;
                while((pos=buffer.find("\n\n"))!=std::string::npos)
                {
                    std::string line=buffer.substr(0,pos);
                    if(line==""||line==":")
                    {
                        buffer.erase(0,pos+2);//删除已处理的行
                        continue;
                    }
                    if(line.compare(0,6,"data: ")==0)
                    {
                        std::string content=line.substr(6,pos);
                        if(content=="[DONE]")
                        {
                           return true;//流式响应结束
                        }
                        //解析响应体
                        //反序列化
                        Json::CharReaderBuilder charBuilder;
                        std::istringstream iss(content);
                        std::string error;//反序列化失败时的错误信息
                        Json::Value responseBody;//反序列化之后的content保存位置
                        bool is_ok=Json::parseFromStream(charBuilder, iss,&responseBody,&error);
                        if(!is_ok)
                        {
                            ERR("Chatgpt model response Deserialization is fails");
                            ERR("Chatgpt model response Deserialization fail error:",error);
                            continue;
                        }
                        //处理响应体
                        if(!responseBody.empty()&&
                           responseBody["input"].isNull()&&
                           !responseBody["input"].empty()&&
                            responseBody["input"][0]["content"].isNull())
                        {
                            std::string delta=responseBody["input"][0]["content"].asString();   
                            Message message;
                            message._role="assistant";
                            message._content=delta;
                            messages.push_back(message);
                            callback(delta,false);
                        }
                        else {
                            WARN("Chatgpt model response is not delta");
                        }
                    }
                    buffer.erase(0,pos+2);//删除已处理的行
                }
               return true;
            };
            //9.发送请求
            auto result=client.send(request);
            return "";
        }
        
}