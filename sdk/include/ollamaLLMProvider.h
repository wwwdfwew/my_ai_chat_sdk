#include "LLMProvider.h"

namespace ai_chat_sdk
{
    class OllamaProviderLLM:public LLMProvider
    {
        public:
            virtual bool ModelInit(std::map<std::string,std::string> config);//模型初始化,配置模型参数  
            virtual bool is_Available();//模型是否可用
            virtual std::string ModelName();//模型名称
            virtual std::string ModelDesc();//模型描述信息
            virtual std::string SendMessage(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams);//以全量返回的方式发送消息,同时消息列表和请求参数引用传递，不然无法修改函数里面的内容。
            virtual std::string SendMessageStream(std::vector<Message>& messages,std::map<std::string,std::string>& requestParams,std::function<void(const std::string&,bool)> callback);
        protected:
            //因为这个模型是部署到本地，而非远端服务器，因此我们不需要配置身份认证的apikey，且我们发送的
            //对模型的请求是先发送到本地的ollama服务器，再由ollama服务器转发给模型服务器，因此我们只需要知道本地的ollama服务端口号
            std::string modelName;//模型名称，部署的模型是根据自己需要，不固定的，因此我们需要模型名称
            std::string modelDesc;//模型描述信息
            bool _is_Available;//模型是否可用
    };
}
