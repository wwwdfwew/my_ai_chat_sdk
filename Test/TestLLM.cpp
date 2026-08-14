#include "../sdk/include/util/my_spdlog.h"   
#include "../sdk/include/DeepseekProvider.h"
#include<gtest/gtest.h>
#include<iostream>
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

int main(int argc,char** argv)
{
    // auto logger=ai_chat_sdk::my_Logger::get_logger();
     ai_chat_sdk::my_Logger::Init("TestLLM",spdlog::level::level_enum::debug,"stdout");
    std::cout<<"TestLLM"<<std::endl;
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
