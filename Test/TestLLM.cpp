#include "../sdk/include/util/my_spdlog.h"   
//#include "../sdk/include/DeepseekProvider.h"
#include<gtest/gtest.h>
#include<iostream>
//#include "../sdk/include/ChatgptProvider.h"
#include"../sdk/include/ollamaLLMProvider.h"
#if 0

TEST(DeepseekProvider,SendMessage)
{
    ai_chat_sdk::DeepseekProvider Provider;
    auto pragma=std::map<std::string,std::string>({{"api_key","sk-8a64a2badb9c4d1980430cf9d9e91df6"}
    ,{"base_url","https://api.deepseek.com"}});
    Provider.ModelInit(pragma);
    ASSERT_TRUE(Provider.is_Available());
    std::vector<ai_chat_sdk::Message> messages;
    ai_chat_sdk::Message message;
    message._role="user";
    message._content="你好,我是张三，你是谁";
    messages.push_back(message);
    auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
    auto callback=[&](const std::string& inf,bool is_end)
    {
        INFO("Deepseek model response delta is:{}",inf);
        if(is_end)
        {
            INFO("[DONE]");
        }
    };
   auto response=Provider.SendMessageStream(messages,requestParams,callback);
   ASSERT_TRUE(!response.empty());
   INFO("Deepseek model response is:{}",response);
    ai_chat_sdk::Message message2;
    message2._role="user";
    message2._content="我是谁";
    messages.push_back(message2);
    response=Provider.SendMessageStream(messages,requestParams,callback);
    ASSERT_TRUE(!response.empty());
    INFO("Deepseek model response is:{}",response);
}


TEST(ChatgptProvider,SendMessage)
{
    ai_chat_sdk::ChatgptProvider Provider;
    auto pragma=std::map<std::string,std::string>();
    pragma["api_key"]=std::getenv("chatgpt_apikey");
    pragma["base_url"]="https://api.openai.com/";
    Provider.ModelInit(pragma);
    ASSERT_TRUE(Provider.is_Available());
    std::vector<ai_chat_sdk::Message> messages;
    ai_chat_sdk::Message message;
    message._role="user";
    message._content="你好,我是张三，你是谁";
    messages.push_back(message);
    auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
    auto callback=[&](const std::string& inf,bool is_end)
    {
        INFO("Chatgpt model response delta is:{}",inf);
        if(is_end)
        {
            INFO("[DONE]");
        }
    };
   auto response=Provider.SendMessage(messages,requestParams);
   ASSERT_TRUE(!response.empty());
   INFO("Chatgpt model response is:{}",response);
    ai_chat_sdk::Message message2;
    message2._role="user";
    message2._content="我是谁";
    messages.push_back(message2);
    response=Provider.SendMessage(messages,requestParams);
    ASSERT_TRUE(!response.empty());
    INFO("Chatgpt model response is:{}",response);
}



TEST(ChatgptProvider,SendMessageStream)
{
    ai_chat_sdk::ChatgptProvider Provider;
    auto pragma=std::map<std::string,std::string>();
    pragma["api_key"]=std::getenv("chatgpt_apikey");
    pragma["base_url"]="https://api.openai.com/";
    Provider.ModelInit(pragma);
    ASSERT_TRUE(Provider.is_Available());
    std::vector<ai_chat_sdk::Message> messages;
    ai_chat_sdk::Message message;
    message._role="user";
    message._content="你好,我是张三，你是谁";
    messages.push_back(message);
    auto requestParams=std::map<std::string,std::string>({{"Temperature","0.7"},{"Max_takens","2048"}});
    auto callback=[&](const std::string& inf,bool is_end)
    {
        INFO("Chatgpt model response delta is:{}",inf);
        if(is_end)
        {
            INFO("[DONE]");
        }
    };
   auto response=Provider.SendMessageStream(messages,requestParams,callback);
   ASSERT_TRUE(!response.empty());
   INFO("Chatgpt model response is:{}",response);
    ai_chat_sdk::Message message2;
    message2._role="user";
    message2._content="我是谁";
    messages.push_back(message2);
    response=Provider.SendMessageStream(messages,requestParams,callback);
    ASSERT_TRUE(!response.empty());
    INFO("Chatgpt model response is:{}",response);
}
#endif
TEST(ollamaLLMProvider,SendMessage)
{
    ai_chat_sdk::OllamaProviderLLM Provider;
    auto pragma=std::map<std::string,std::string>();
    pragma["modelName"]="deepseek-r1:1.5b";
    pragma["modelDesc"]="这是由深度求索公司开发的思考推理模型——deepseek-r1:1.5b";
    Provider.ModelInit(pragma);
    ASSERT_TRUE(Provider.is_Available());
    std::vector<ai_chat_sdk::Message> messages;
    ai_chat_sdk::Message message;
    message._role="user";
    message._content="你好,我是张三，你是谁";
    messages.push_back(message);
    auto requestParams=std::map<std::string,std::string>({{"temperature","0.7"},{"num_ctx","2048"}});
    auto callback=[&](const std::string& inf,bool is_end)
    {
        INFO("Chatgpt model response delta is:{}",inf);
        if(is_end)
        {
            INFO("[DONE]");
        }
    };
   auto response=Provider.SendMessage(messages,requestParams);
   ASSERT_TRUE(!response.empty());
   INFO("deepseek-r1:1.5b model response is:{}",response);
    ai_chat_sdk::Message message2;
    message2._role="user";
    message2._content="我是谁";
    messages.push_back(message2);
    response=Provider.SendMessage(messages,requestParams);
    ASSERT_TRUE(!response.empty());
    INFO("deepseek-r1:1.5b model response is:{}",response);
}

int main(int argc,char** argv)
{
    // auto logger=ai_chat_sdk::my_Logger::get_logger();
     ai_chat_sdk::my_Logger::Init("TestLLM",spdlog::level::level_enum::debug,"stdout");
    std::cout<<"TestLLM"<<std::endl;
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
