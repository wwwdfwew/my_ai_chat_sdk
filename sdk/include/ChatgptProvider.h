#include"LLMProvider.h"

namespace ai_chat_sdk
{
    class ChatgptProvider:public LLMProvider
    {
        public:
            virtual bool ModelInit(std::map<std::string,std::string> config);//模型初始化,配置模型参数  
            virtual bool is_Available();//模型是否可用
            virtual std::string ModelName();//模型名称
            virtual std::string ModelDesc();//模型描述信息
            virtual std::string SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams);//以全量返回的方式发送消息,同时消息列表和请求参数引用传递，不然无法修改函数里面的内容。
            virtual std::string SendMessageStream(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams,std::function<void(const std::string&,bool)> callback);
        protected:
            std::string api_key;//模型Api_key
            std::string base_url;//模型基础URL
            bool is_Available_=false;//模型是否可用
    };
}