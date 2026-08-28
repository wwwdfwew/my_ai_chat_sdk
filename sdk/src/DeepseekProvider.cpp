#include"../include/DeepseekProvider.h"
#include<httplib.h>
#include<jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include"../include/util/my_spdlog.h"
namespace ai_chat_sdk
{
    bool DeepseekProvider::ModelInit(std::map<std::string,std::string> config)
    {
        //配置api_key
       auto it=config.find("api_key");
       if(it==config.end())
       {
        ERR("Deepseek api_key is not find");
        is_Available_=false;
       }
       else
       {
        api_key=it->second;
        is_Available_=true;
       }
       //配置base_url
        it=config.find("base_url");
       if(it==config.end())
       {
       ERR("Deepseek base_url is not find");
        is_Available_=false;
       }
       else
       {
        base_url=it->second;
        is_Available_=true;
       }
       return is_Available_;
    }
    bool DeepseekProvider::is_Available()
    {
        return is_Available_;
    }
    std::string DeepseekProvider::ModelName()
    {
        return "deepseek-v4-pro";
    }
    std::string DeepseekProvider::ModelDesc()
    {
        return "这是由深度求索公司开发的一款大语言模型——deepseek";
    }
    std::string DeepseekProvider::SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams)
    {
        //1.检查模型是否可用
        if(!is_Available_)
        {
            ERR("Deepseek model is not available");
            return "";
        }
        //2.构造请求参数
        double Temperature=0.7;
        int Max_takens=2048;
        if(requestParams.find("Temperature")!=requestParams.end())
        {
            Temperature=stod(requestParams["Temperature"]);//stod将字符串转换为double类型
        }
        if(requestParams.find("Max_takens")!=requestParams.end())
        {
            Max_takens=stoi(requestParams["Max_takens"]);//stoi将字符串转换为int类型
        }

        //3.历史消息：因为http协议是无状态的，每一次请求和响应都是独立的，因此，模型是对之前每一次发送的内容没有记忆的，我们需要在请求中包含历史消息参数。
        //用json格式的数组存储历史消息，数组里面存储的都是一个一个对象
        Json::Value messageArray(Json::arrayValue);
        for(auto& message: messages)
        {
           Json::Value messageobject;
           messageobject["role"]=message._role;
           messageobject["content"]=message._content;
           messageArray.append(messageobject);
        }

        //4.构造请求体
        Json::Value requestBody;    
        requestBody["model"]=ModelName();
        requestBody["messages"]=messageArray;
        requestBody["temperature"]=Temperature;
        requestBody["max_tokens"]=Max_takens;

        //5.序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr=Json::writeString(writerBuilder,requestBody);
        
        //6.构建请求头：
        httplib::Headers header={{"Authorization","Bearer "+api_key},{"Content-Type","application/json"}};
        //7..构建http客户端
        httplib::Client client(base_url.c_str());
        //设置发送超时时间
        //client.set_max_timeout(30);//这个是最大超时时间，包括连接和读取响应时间，有可能响应时间超过30秒，导致超时，所以一直无法成功。
        client.set_connection_timeout(30);
        //设置响应超时时间
        client.set_read_timeout(60);
        //使用post方法发送消息
        auto response=client.Post("/chat/completions",header,requestBodyStr,"application/json");
        if(!response)
        {
                //写成这样的错误方式导致一直运行失败。
                //  ERR("Deepseek model response is error:{}",response->reason);
                // ERR("Deepseek model response status is:{}",response->status);
            ERR("Deepseek model response is error");
            return "";
        }
         ERR("Deepseek model response is success:{}",response->body);
         ERR("Deepseek model response status is:{}",response->status);
        if(response->status!=200)
        {
            ERR("Deepseek model response status is not 200");
            ERR("Deepseek model response status is:{}",response->status);
            return "";
        }
       
      //8.解析响应体
      //反序列化
      Json::CharReaderBuilder creadBuilder;
      Json::Value responseJson;
      std::string errorMsg;
      std::istringstream iss(response->body);
      bool success=Json::parseFromStream(creadBuilder,iss,&responseJson,&errorMsg);
      if(!success)
      {
          ERR("Deepseek model response is not valid json:{}",errorMsg);
          return "";   
      }
      
      if(responseJson.isMember("choices")&&!responseJson["choices"].empty()&&responseJson["choices"].isArray())    
      {
        Message message;
       message._role=responseJson["choices"][0]["message"]["role"].asString();
       message._content=responseJson["choices"][0]["message"]["content"].asString();
       //9.将模型回复添加到历史消息中
       messages.push_back(message); //这也是为什么std::vector<Message> messages要加引用才能实现在TestLLM.cpp中修改TestLLM.cpp中的message.
       INFO("Deepseek model response is:{}",message._content);
       return message._content;     
      }
        return "";
    }
    std::string DeepseekProvider::SendMessageStream(std::vector<Message>& messages,
                                                    std::map<std::string,std::string>& requestParams,
                                                    std::function<void(const std::string&,bool)> callback)
    {
        INFO("=== Enter SendMessageStream ===");
        //1.检查模型是否可用
        if(!is_Available_)
        {
            ERR("Deepseek model is not available");
            return "";
        }
        //2.构造请求参数
        double Temperature=0.7;
        int Max_takens=2048;
        if(requestParams.find("Temperature")!=requestParams.end())
        {
            Temperature=stod(requestParams["Temperature"]);//stod将字符串转换为double类型
        }
        if(requestParams.find("Max_takens")!=requestParams.end())
        {
            Max_takens=stoi(requestParams["Max_takens"]);//stoi将字符串转换为int类型
        }
        //3.添加历史消息
        Json::Value messageArray(Json::arrayValue);
        for(auto& message: messages)
        {
           Json::Value messageobject;
           messageobject["role"]=message._role;
           messageobject["content"]=message._content;
           messageArray.append(messageobject);
        }
        //4.构造请求体
        Json::Value requestBody;    
        requestBody["model"]=ModelName();
        requestBody["messages"]=messageArray;
        requestBody["temperature"]=Temperature;
        requestBody["max_tokens"]=Max_takens;
        requestBody["stream"]=true;//开启流式返回
        
        //5.序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr=Json::writeString(writerBuilder,requestBody);
        //6.构建请求头
        httplib::Headers header={{"Authorization","Bearer "+api_key},
                                {"Content-Type","application/json"},
                                {"Accept","text/event-stream"}};//流式返回参数
        
        //7.构建http客户端
        httplib::Client client(base_url);
        //设置连接超时时间
        client.set_connection_timeout(30,0);
        //设置响应超时时间
        client.set_read_timeout(300,0);
        //8.构建请求对象
        httplib::Request request;
        request.method="POST";
        request.body=requestBodyStr;
        request.headers=header;
        request.path="/chat/completions";

        
        std::string buffer;
        bool is_success=false;
        std::string full_response;

        
        //9.构建响应处理器
        request.response_handler=[&](const httplib::Response& response)
        {
            if(response.status==200)
            {
                is_success=true;
                INFO("Deepseek model response is success:{}",response.status);
                return true;
            }
            else
            {
                is_success=false;
                ERR("Deepseek model response is error");
                return false;
            }
        };
        //10.构建数据处理函数
        request.content_receiver=[&](const char *data, size_t data_length, size_t offset, size_t total_length)
        {
            //是否响应成功
            if(!is_success)
            {
                INFO("Deepseek model response is success");
                return false;
            }
            //将这一次响应的数据追加到buffer中
            buffer.append(data,data_length);
            //这里是将响应数据追加到buffer里，因为网络传输是分包的，因此每一次响应的数据可能不完整，但是SSE的格式是知道的。
            //因此我们将每一次响应的数据都添加到buffer中，在解析数据时，根据SSE协议中数据块是以\n\n分割，所以如果这次的数据中没有\n\n
            //那么就不会处理，下一次添加数据时，会继续添加到buffer中，直到遇到\n\n，才会解析
            //最后解析完成后，会将buffer中的数据清空，继续下一次解析
            
            //解析数据
            size_t pos=0;
            while((pos=buffer.find("\n\n"))!=std::string::npos)
            {
                //INFO("Deepseek model response comtent_receiver pos is");
                //将该数据块根据\n\n切割
                std::string Date=buffer.substr(0,pos);
                //解析数据块
                //1.除去空行和注释
                if(Date.empty()||Date==":")
                {
                    buffer.erase(0,pos+2);
                    continue;
                }
                //2.比较前缀是否是data:
                if(Date.compare(0,6,"data: ")==0)
                {
                    //截取data:后面的内容
                    std::string data=Date.substr(6);
                    //判断内容是否为流式响应结束
                    if(data=="[DONE]")
                    {
                       return true;
                    }
                       //对内容反序列化
                       Json::Value responseBody;
                       Json::CharReaderBuilder readerBuilder;
                       std::istringstream iss(data);
                       std::string errs;
                       bool is_parse=Json::parseFromStream(readerBuilder, iss, &responseBody, &errs);
                       if(!is_parse)
                       {
                           ERR("Deepseek model respnse parse error:{}",errs);
                           return false;
                       }
                       //获取choice字段中的delta
                        if(responseBody.isMember("choices")&&
                          !responseBody["choices"].empty()&&
                          responseBody["choices"].isArray()&&
                          responseBody["choices"][0].isMember("delta")&&
                          responseBody["choices"][0]["delta"].isMember("content"))
                          {
                            std::string content=responseBody["choices"][0]["delta"]["content"].asString();
                            callback(content,false);
                            full_response.append(content);
                          }
                        else {
                            WARN("Deepseek model respnse is not delta,{},{},{},{},{},{}",responseBody.isMember("choices"),!responseBody["choices"].empty(),responseBody["choices"].isArray(),responseBody["choices"][0].isMember("delta"),responseBody["choices"][0]["delta"].isMember("content"),!responseBody["choices"][0]["delta"]["content"].empty());
                        }   
                         buffer.erase(0,pos+2);
                }
            }
            return true;
        };
        auto result=client.send(request);
        if(!result)
        {
            ERR("Deepseek model request internet error");
            return "";
        }

        if(!is_success)
        {
            INFO("Deepseek model response is over");//模型流式返回结束
            callback("",true);
            return "";
        }
        return full_response;
    }                                             
}
