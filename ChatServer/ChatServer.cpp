#include "ChatServer.h"
#include<jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include<ai_chat_sdk/util/my_spdlog.h>
namespace ai_chat_server
{
    //构造函数并且初始化
    ChatServer::ChatServer(const ChatServerConfig& config)
    {
        //创建chatsdk实例对象
        sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    //模型的参数信息：
    //deepseek-chat模型参数
    auto deepseek_cahtConfigs=std::make_shared<ai_chat_sdk::APIConfig>();
    deepseek_cahtConfigs->ModelName="deepseek-chat";
    deepseek_cahtConfigs->Temperature=config.temperature;
    deepseek_cahtConfigs->Max_takens=config.max_takens;
    deepseek_cahtConfigs->Api_key=std::getenv("deepseek_apikey");
 
    //gpt-4o-mini模型参数
    auto gpt4o_cahtConfigs=std::make_shared<ai_chat_sdk::APIConfig>();
    gpt4o_cahtConfigs->ModelName="gpt-4o-mini";
    gpt4o_cahtConfigs->Temperature=config.temperature;
    gpt4o_cahtConfigs->Max_takens=config.max_takens; 
    gpt4o_cahtConfigs->Api_key=std::getenv("chatgpt_apikey");
 
    //ollama中的deepseek-r1:1.5b模型参数
    auto deepseek_r1_5b_cahtConfigs=std::make_shared<ai_chat_sdk::ollamaConfig>();
    deepseek_r1_5b_cahtConfigs->ModelName=config.ollamaModelName;
    deepseek_r1_5b_cahtConfigs->ModelDesc=config.ollamaModelDesc;
    deepseek_r1_5b_cahtConfigs->base_url=config.ollamaBaseUrl;
    deepseek_r1_5b_cahtConfigs->Temperature=config.temperature;
    deepseek_r1_5b_cahtConfigs->Max_takens=config.max_takens; 

    //将所有模型配置信息添加到一个vector中
    std::vector<std::shared_ptr<ai_chat_sdk::ModelConfig>> configs;
    configs.push_back(deepseek_cahtConfigs);
    configs.push_back(gpt4o_cahtConfigs);
    configs.push_back(deepseek_r1_5b_cahtConfigs);

    //初始化SDK
    sdk->ChatSDKInit(configs);


    //创建服务器对象
    server_ = std::make_unique<httplib::Server>();
    this->config=config;
    }

     //启动服务器
    bool ChatServer::Start()
    {
       //判断服务器是否正在运行
       if(is_running_.load())
       {
           INFO("the server is running already");
           return false;
       }

       // 设置路由规则
        setHttpRoutes();

        // 设置静态资源的路径
        // 前端页面相关的所有文件都放在www目录下  注意：将来前端页面名称命名为index.html
        // 当用户在浏览器中输入：http://ip:port/index.html    http://ip:port也能访问index.html页面
        // 在httplib中，默认情况下，如果请求路径中只有ip和端口，httplib默认会使用index.html文件
        server_->set_mount_point("/", "./www");

       //为了保证不卡主线程，单独开一个线程启动服务器
       std::thread thread([this](){
           server_->listen(config.host, config.port);
       });
       thread.detach();
       is_running_.store(true);
       return true;
    }

     //停止服务器
    void ChatServer::Stop()
    {
        if(!is_running_.load())
        {
            INFO("the server is not running");
            return;
        }
        if(server_)
        {
            server_->stop();
        }
        is_running_.store(false);
        INFO("the server is stopped");
    }

     //判断服务器是否正在运行
    bool ChatServer::IsRunning() const
    {
        return is_running_.load();
    }

    //处理响应错误信息
    std::string ChatServer::HandleErrorResponse(const std::string& message)
    {
        Json::Value responseJson;
        responseJson["success"]=false;
        responseJson["message"]=message;
        //将响应体序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        return responseJsonStr;
    }

    //获取会话列表
     void ChatServer::handleGetSessionListsRequest(const httplib::Request& request, httplib::Response& response)
     {
        //请求的是会话列表，所以将session中的会话列表响应给客户端
        //会话列表存储在Json Value数组中
        Json::Value sessionArray(Json::arrayValue);
        //通过sdk获取会话列表
        auto sessionLists=sdk->ChatSDKGetSessionLists();
        //通过会话id拿到会话对象信息，再添加到数组中
        for(auto& session:sessionLists)
        {
           //先拿到指定会话id的会话对象信息
           auto sessionObj=sdk->ChatSDKGetSession(session);
           //创建一个Json Value对象
           Json::Value sessionJson;
           sessionJson["id"]=sessionObj->session_id;
           sessionJson["model"]=sessionObj->ModelName;
           sessionJson["create_time"]=sessionObj->create_time;
           sessionJson["update_time"]=sessionObj->update_time;
           sessionJson["message_count"]=sessionObj->messages.size();
           if(!sessionObj->messages.empty())
           {
               sessionJson["first_user_message"]=sessionObj->messages.front()._content;
           }
           //将会话对象添加到数组中
           sessionArray.append(sessionJson);
        }

        //构建响应体
        Json::Value dataJson;
        dataJson["array"]=sessionArray;
        
        Json::Value responseJson;
        responseJson["success"]=true;
        responseJson["message"]="get session lists success";
        responseJson["data"]=dataJson;

        //序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        //设置响应状态码
        response.status=200;
        response.set_content(responseJsonStr,"application/json");
     }


     //创建会话
     void ChatServer::handleCreateSessionRequest(const httplib::Request& request, httplib::Response& response)
     {
        //将请求正文反序列化
        Json::Reader reader;
        Json::Value requestJson;
        if(!reader.parse(request.body, requestJson))
        {
            //反序列化失败，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("parse failed,request body is not valid");
            response.status=400;
            response.set_content(responseJsonStr,"application/json");
            return;
        }
        //反序列化成功
        //获取会话模型名称
        std::string modelName=requestJson.get("model","deepseek-chat").asString();//未找到model字段，默认使用deepseek-chat
        //创建会话
        auto SessionID=sdk->ChatSDKCreateSession(modelName);
        if(SessionID.empty())
        {
            //创建会话失败，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("create session failed,server internal error");
            response.status=500;
            response.set_content(responseJsonStr,"application/json");
            return;
        }

        //创建会话成功，返回成功响应信息
        Json::Value dataJson;
        dataJson["session_id"]=SessionID;
        dataJson["model"]=modelName;

        Json::Value responseJson;
        responseJson["success"]=true;
        responseJson["message"]="create session success";
        responseJson["data"]=dataJson;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        //设置响应状态码
        response.status=200;
        response.set_content(responseJsonStr,"application/json");
     }



     //获取可用模型
     void ChatServer::handleGetAvailableModelsRequest(const httplib::Request& request, httplib::Response& response)
     {
        //同样的，直接返回可用的模型列表
        auto availableModels=sdk->ChatSDKGetAvailableModels();
        Json::Value ModelInfoArray(Json::arrayValue);
        for(auto& modelinfo:availableModels)
        {
            Json::Value modelJson;
            modelJson["name"]=modelinfo.ModelName;
            modelJson["desc"]=modelinfo.ModelDesc;
            //将模型对象添加到数组中
            ModelInfoArray.append(modelJson);
        }
        //构建响应体
        Json::Value dataJson;
        dataJson["array"]=ModelInfoArray;
        Json::Value responseJson;
        responseJson["success"]=true;
        responseJson["message"]="get available models success";
        responseJson["data"]=dataJson;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        //设置响应状态码
        response.status=200;
        response.set_content(responseJsonStr,"application/json");
     }
     //获取历史消息
     void ChatServer::handleGetHistoryMessagesRequest(const httplib::Request& request, httplib::Response& response)
     {
        //通过提交的请求中找到会话id
        std::string session_id=request.matches[1];
        //获取会话
        auto session=sdk->ChatSDKGetSession(session_id);
        if(!session)
        {
            //会话不存在，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("session not found");
            response.status=404;
            response.set_content(responseJsonStr,"application/json");
            return;
        }

        //构建历史消息列表
       Json::Value messageArray(Json::arrayValue);
       for(auto message:session->messages)
       {
           Json::Value messageJson;
           messageJson["id"]=message._message_id;
           messageJson["content"]=message._content;
           messageJson["role"]=message._role;
           messageJson["timestamp"]=static_cast<int64_t>(message._create_time);
           //将消息对象添加到数组中
           messageArray.append(messageJson);
       }
       
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get history messages success";
        responseJson["data"] = messageArray;

        // 序列化
        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200; // 成功
        response.set_content(responseJsonStr, "application/json");

     }
     //发送消息，全量返回
     void ChatServer::handleSendMessageRequest(const httplib::Request& request, httplib::Response& response)
     {
        //序列化请求体为json对象
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(request.body, requestJson))
        {
            //解析请求体失败，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("parse request body failed");
            response.status=400;
            response.set_content(responseJsonStr,"application/json");
            return;
        }

        //获取请求体中的参数
        auto session_id=requestJson["session_id"].asString();
        auto message=requestJson["message"].asString();
        if(session_id.empty()||message.empty())
        {
            //会话id或消息为空，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("session_id or message is empty");
            response.status=400;
            response.set_content(responseJsonStr,"application/json");
            return;
        }

        //发送消息
        auto ret=sdk->sendMessage(session_id,message);
        if(ret.empty())
        {
            //发送消息失败，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("send message failed");
            response.status=500;
            response.set_content(responseJsonStr,"application/json");
            return;
        }
        //发送消息成功，返回成功响应
       response.status=200;
      //构建响应体
      Json::Value dataJson;
      dataJson["session_id"]=session_id;
      dataJson["response"]=ret;
      Json::Value responseJson;
      responseJson["success"]=true;
      responseJson["message"]="send message success";
      responseJson["data"]=dataJson;



      //序列化
      Json::StreamWriterBuilder writerBuilder;
      std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
      response.set_content(responseJsonStr,"application/json");
     }
     //发送消息，流式返回
     void ChatServer::handleSendMessageStreamRequest(const httplib::Request& request, httplib::Response& response)
     {
        //序列化请求体为json对象
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(request.body, requestJson))
        {
            //解析请求体失败，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("parse request body failed");
            response.status=400;
            response.set_content(responseJsonStr,"application/json");
            return;
        }

        //获取请求体中的参数
        auto session_id=requestJson["session_id"].asString();
        auto message=requestJson["message"].asString();
        if(session_id.empty()||message.empty())
        {
            //会话id或消息为空，返回失败响应
            std::string responseJsonStr = HandleErrorResponse("session_id or message is empty");
            response.status=400;
            response.set_content(responseJsonStr,"application/json");
            return;
        }


        //准备流式响应
        response.status=200;
        response.set_header("Cache-Control", "no-cache");              // 不使用缓存，服务器立即将数据发送到网络
        response.set_header("Connection", "keep-alive");               // 保持连接，服务器不会关闭连接

         response.set_chunked_content_provider("text/event-stream", [this, session_id, message](size_t offset, httplib::DataSink& dataSink)->bool{

        auto writeChunk = [&](const std::string& chunk, bool last){ 
            // 将chunk转换为SSE数据格式
            // Json::valueToQuotedString: 对chunk进行Json转换，目的防止chunk中包含一些特殊字符来破坏数据格式，比如：在chunk中包含了两个连续的换行，就会影响SSE数据格式
            std::string sseData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";

            // 需要将模型返回的结果 chunk 发送给客户单
            dataSink.write(sseData.c_str(), sseData.size());  // 将数据写入响应流，即立即发送给客户单，该方法不会等待缓冲区满之后发送

            // 处理结束标记
            if(last){
                // 流向响应结束
                std::string doneData = "data: [DONE]\n\n";
                dataSink.write(doneData.c_str(), doneData.size());
                dataSink.done();    // 表示流式响应结束
                return false;       // 不再有后续数据
            }
            return true;
        };
        
        // 先给客户端发送一个空的数据块，避免客户端长时间的等待
        if (!writeChunk("", false)) {
            return false;
        }
        
        // 发送消息流
        sdk->sendMessageStream(session_id, message, writeChunk);

        return false;   // 不再有后续数据
    });
     }
     //删除会话
     void ChatServer::handleDeleteSessionRequest(const httplib::Request& request, httplib::Response& response)
     {
         std::string sessionId = request.matches[1];
         bool ret=sdk->ChatSDKDeleteSession(sessionId);
         if(ret)
         {
            //删除会话成功
            //返回成功响应
            Json::Value responseJson;
            responseJson["success"]=true;
            responseJson["message"]="delete session success";

            response.status=200;
            //序列化
            Json::StreamWriterBuilder writerBuilder;
            std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
            response.set_content(responseJsonStr,"application/json");
         }
         else
         {
            //删除会话失败,会话不存在
            std::string responseJsonStr = HandleErrorResponse("delete failed, session not found");
            response.status=404;
            response.set_content(responseJsonStr,"application/json");
         }
     }

      void ChatServer::setHttpRoutes()
      {
        //创建会话
         server_->Post("/api/session", [this](const httplib::Request& request, httplib::Response& response){
        handleCreateSessionRequest(request, response);});

         //获取可用模型
         server_->Get("/api/models", [this](const httplib::Request& request, httplib::Response& response){
        handleGetAvailableModelsRequest(request, response);});

        //获取历史消息
        server_->Get(R"(/api/session/(.*)/history)", [this](const httplib::Request& request, httplib::Response& response){
        handleGetHistoryMessagesRequest(request, response);});

        //发送消息，全量返回
        server_->Post("/api/message", [this](const httplib::Request& request, httplib::Response& response){
        handleSendMessageRequest(request, response);});

        //发送消息，流式返回
        server_->Post("/api/message/async", [this](const httplib::Request& request, httplib::Response& response){
        handleSendMessageStreamRequest(request, response);});

        //删除会话
        server_->Delete(R"(/api/session/(.*))", [this](const httplib::Request& request, httplib::Response& response){
        handleDeleteSessionRequest(request, response);});
        //获取会话列表
        server_->Get("/api/sessions", [this](const httplib::Request& request, httplib::Response& response){
        handleGetSessionListsRequest(request, response);});

      }

}