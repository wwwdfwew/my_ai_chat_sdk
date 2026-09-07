
#include "../sdk/include/util/my_spdlog.h"   
#include "../sdk/include/DeepseekProvider.h"
#include<gtest/gtest.h>
#include<iostream>
#include "../sdk/include/ChatgptProvider.h"
#include"../sdk/include/ollamaLLMProvider.h"
#include "../sdk/include/ChatSDK.h"
#include<memory>
#include "../sdk/include/common.h"


// #if 0

// TEST(DeepseekProvider,SendMessage)
// {
//     ai_chat_sdk::DeepseekProvider Provider;
//     auto pragma=std::map<std::string,std::string>();
//     pragma["api_key"]=std::getenv("deepseek_apikey");
//     pragma["base_url"]="https://api.deepseek.com";
//     Provider.ModelInit(pragma);
//     ASSERT_TRUE(Provider.is_Available());
//     std::vector<ai_chat_sdk::Message> messages;
//     ai_chat_sdk::Message message;
//     message._role="user";
//     message._content="你好,我是张三，你是谁";
//     messages.push_back(message);
//     auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
//     auto callback=[&](const std::string& inf,bool is_end)
//     {
//         INFO("Deepseek model response delta is:{}",inf);
//         if(is_end)
//         {
//             INFO("[DONE]");
//         }
//     };
//    auto response=Provider.SendMessageStream(messages,requestParams,callback);
//    ASSERT_TRUE(!response.empty());
//    INFO("Deepseek model response is:{}",response);
//     ai_chat_sdk::Message message2;
//     message2._role="user";
//     message2._content="我是谁";
//     messages.push_back(message2);
//     response=Provider.SendMessageStream(messages,requestParams,callback);
//     ASSERT_TRUE(!response.empty());
//     INFO("Deepseek model response is:{}",response);
// }


// TEST(ChatgptProvider,SendMessage)
// {
//     ai_chat_sdk::ChatgptProvider Provider;
//     auto pragma=std::map<std::string,std::string>();
//     pragma["api_key"]=std::getenv("chatgpt_apikey");
//     pragma["base_url"]="https://api.openai.com/";
//     Provider.ModelInit(pragma);
//     ASSERT_TRUE(Provider.is_Available());
//     std::vector<ai_chat_sdk::Message> messages;
//     ai_chat_sdk::Message message;
//     message._role="user";
//     message._content="你好,我是张三，你是谁";
//     messages.push_back(message);
//     auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
//     auto callback=[&](const std::string& inf,bool is_end)
//     {
//         INFO("Chatgpt model response delta is:{}",inf);
//         if(is_end)
//         {
//             INFO("[DONE]");
//         }
//     };
//    auto response=Provider.SendMessage(messages,requestParams);
//    ASSERT_TRUE(!response.empty());
//    INFO("Chatgpt model response is:{}",response);
//     ai_chat_sdk::Message message2;
//     message2._role="user";
//     message2._content="我是谁";
//     messages.push_back(message2);
//     response=Provider.SendMessage(messages,requestParams);
//     ASSERT_TRUE(!response.empty());
//     INFO("Chatgpt model response is:{}",response);
// }



// TEST(ChatgptProvider,SendMessageStream)
// {
//     ai_chat_sdk::ChatgptProvider Provider;
//     auto pragma=std::map<std::string,std::string>();
//     pragma["api_key"]=std::getenv("chatgpt_apikey");
//     pragma["base_url"]="https://api.openai.com/";
//     Provider.ModelInit(pragma);
//     ASSERT_TRUE(Provider.is_Available());
//     std::vector<ai_chat_sdk::Message> messages;
//     ai_chat_sdk::Message message;
//     message._role="user";
//     message._content="你好,我是张三，你是谁";
//     messages.push_back(message);
//     auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
//     auto callback=[&](const std::string& inf,bool is_end)
//     {
//         INFO("Chatgpt model response delta is:{}",inf);
//         if(is_end)
//         {
//             INFO("[DONE]");
//         }
//     };
//    auto response=Provider.SendMessageStream(messages,requestParams,callback);
//    ASSERT_TRUE(!response.empty());
//    INFO("Chatgpt model response is:{}",response);
//     ai_chat_sdk::Message message2;
//     message2._role="user";
//     message2._content="我是谁";
//     messages.push_back(message2);
//     response=Provider.SendMessageStream(messages,requestParams,callback);
//     ASSERT_TRUE(!response.empty());
//     INFO("Chatgpt model response is:{}",response);
// }

// TEST(ollamaLLMProvider,SendMessage)
// {
//     ai_chat_sdk::OllamaProviderLLM Provider;
//     auto pragma=std::map<std::string,std::string>();
//     pragma["modelName"]="deepseek-r1:1.5b";
//     pragma["modelDesc"]="这是由深度求索公司开发的思考推理模型——deepseek-r1:1.5b";
//     Provider.ModelInit(pragma);
//     ASSERT_TRUE(Provider.is_Available());
//     std::vector<ai_chat_sdk::Message> messages;
//     ai_chat_sdk::Message message;
//     message._role="user";
//     message._content="你好,我是张三，你是谁";
//     messages.push_back(message);
//     auto requestParams=std::map<std::string,std::string>({{"temperature","0.7"},{"num_ctx","2048"}});
//     auto callback=[&](const std::string& inf,bool is_end)
//     {
//         INFO("Chatgpt model response delta is:{}",inf);
//         if(is_end)
//         {
//             INFO("[DONE]");
//         }
//     };
//    auto response=Provider.SendMessage(messages,requestParams);
//    ASSERT_TRUE(!response.empty());
//    INFO("deepseek-r1:1.5b model response is:{}",response);
//     ai_chat_sdk::Message message2;
//     message2._role="user";
//     message2._content="我是谁";
//     messages.push_back(message2);
//     response=Provider.SendMessage(messages,requestParams);
//     ASSERT_TRUE(!response.empty());
//     INFO("deepseek-r1:1.5b model response is:{}",response);
// }


// TEST(ollamaLLMProvider,SendMessage)
// {
//     ai_chat_sdk::OllamaProviderLLM Provider;
//     auto pragma=std::map<std::string,std::string>();
//     pragma["modelName"]="deepseek-r1:1.5b";
//     pragma["modelDesc"]="这是由深度求索公司开发的思考推理模型——deepseek-r1:1.5b";
//     Provider.ModelInit(pragma);
//     ASSERT_TRUE(Provider.is_Available());
//     std::vector<ai_chat_sdk::Message> messages;
//     ai_chat_sdk::Message message;
//     message._role="user";
//     message._content="你是谁";
//     messages.push_back(message);
//     auto requestParams=std::map<std::string,std::string>({{"temperature","0.7"},{"num_ctx","2048"}});
//     auto callback=[&](const std::string& inf,bool is_end)
//     {
//         INFO("{} model response content is:{}",Provider.ModelName(),inf);
//         if(is_end)
//         {
//             INFO("[DONE]");
//         }
//     };
//    auto response=Provider.SendMessageStream(messages,requestParams,callback);
//    ASSERT_TRUE(!response.empty());
//    INFO("{} model response is:{}",Provider.ModelName(),response);
    // ai_chat_sdk::Message message2;
    // message2._role="user";
    // message2._content="我是谁";
    // messages.push_back(message2);
    // response=Provider.SendMessageStream(messages,requestParams,callback);
    // ASSERT_TRUE(!response.empty());
    // INFO("{} model response is:{}",Provider.ModelName(),response);
//}

// 测试ChatSDK
TEST(ChatSDK,SendMessage)
{
    //创建chatsdk实例对象
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);
    //模型的参数信息：
    //deepseek-chat模型参数
    auto deepseek_cahtConfigs=std::make_shared<ai_chat_sdk::APIConfig>();
     ASSERT_TRUE(deepseek_cahtConfigs != nullptr);
    deepseek_cahtConfigs->ModelName="deepseek-chat";
    deepseek_cahtConfigs->Temperature=0.7;
    deepseek_cahtConfigs->Max_takens=2048;
    deepseek_cahtConfigs->Api_key=std::getenv("deepseek_apikey");
 
    //gpt-4o-mini模型参数
    auto gpt4o_cahtConfigs=std::make_shared<ai_chat_sdk::APIConfig>();
     ASSERT_TRUE(gpt4o_cahtConfigs != nullptr);
    gpt4o_cahtConfigs->ModelName="gpt-4o-mini";
    gpt4o_cahtConfigs->Temperature=0.7;
    gpt4o_cahtConfigs->Max_takens=2048; 
    gpt4o_cahtConfigs->Api_key=std::getenv("chatgpt_apikey");
 
    //ollama中的deepseek-r1:1.5b模型参数
    auto deepseek_r1_5b_cahtConfigs=std::make_shared<ai_chat_sdk::ollamaConfig>();
     ASSERT_TRUE(deepseek_r1_5b_cahtConfigs != nullptr);
    deepseek_r1_5b_cahtConfigs->ModelName="deepseek-r1:1.5b";
    deepseek_r1_5b_cahtConfigs->ModelDesc="这是由深度求索公司开发的思考推理模型——deepseek-r1:1.5b";
    deepseek_r1_5b_cahtConfigs->base_url="http://localhost:11434";
    deepseek_r1_5b_cahtConfigs->Temperature=0.7;
    deepseek_r1_5b_cahtConfigs->Max_takens=2048; 

    //将所有模型配置信息添加到一个vector中
    std::vector<std::shared_ptr<ai_chat_sdk::ModelConfig>> configs;
    configs.push_back(deepseek_cahtConfigs);
    configs.push_back(gpt4o_cahtConfigs);
    configs.push_back(deepseek_r1_5b_cahtConfigs);

    //初始化SDK
    sdk->ChatSDKInit(configs);
    //创建会话
    auto session_id=sdk->ChatSDKCreateSession("deepseek-chat");
    ASSERT_TRUE(!session_id.empty());
    INFO("session {} created",session_id);
    //发送消息
    auto callback=[&](const std::string& inf,bool is_end)
    {
        INFO("deepseek-chat model response delta is:{}",inf);
        if(is_end)
        {
            INFO("[DONE]");
        }
    };
    std::string message="你好,我是张三，你是谁";
    auto response=sdk->sendMessageStream(session_id,message,callback);
    ASSERT_TRUE(!response.empty());
    INFO("deepseek-chat model response is:{}",response);

    //chatgpt需要打开代理
    // auto gpt_session_id=sdk->ChatSDKCreateSession("gpt-4o-mini");
    // ASSERT_TRUE(!gpt_session_id.empty());
    // INFO("session {} created",gpt_session_id);
    // //发送消息

    // std::string message1="你好,我是张三，你是谁";
    // auto gpt_response=sdk->sendMessage(gpt_session_id,message1);
    // ASSERT_TRUE(!gpt_response.empty());
    // INFO("gpt-4o-mini model response is:{}",gpt_response);


    auto ollama_session_session_id=sdk->ChatSDKCreateSession("deepseek-r1:1.5b");
    ASSERT_TRUE(!ollama_session_session_id.empty());
    INFO("session {} created",ollama_session_session_id);
    //发送消息

    std::string message2="你好,我是张三，你是谁";
    auto ollama_response=sdk->sendMessage(ollama_session_session_id,message2);  
    ASSERT_TRUE(!ollama_response.empty());
    INFO("deepseek-r1:1.5b model response is:{}",ollama_response);


}

int main(int argc,char** argv)
{
    // auto logger=ai_chat_sdk::my_Logger::get_logger();
     ai_chat_sdk::my_Logger::Init("TestLLM",spdlog::level::level_enum::debug,"stdout");
    std::cout<<"TestLLM"<<std::endl;
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
