//会话管理类
#pragma once
#include"common.h"
#include <memory>
#include<mutex>
#include<unordered_map>
#include<atomic>
#include<vector>


namespace ai_chat_sdk {
class SessionManage {
public:
//创建会话，在创建会话之前，先要选择你要使用的模型，然后创建会话
std::string  CreateSession(const std::string& model_name);
//通过会话id获取指定会话对象
std::shared_ptr<Session> GetSession(const std::string& session_id)const;
//向某会话中添加消息
bool AddMessage(const std::string& session_id,const Message& message)const;
//添加了消息就要更新会话时间戳,根据会话id更新对应的会话时间戳
bool UpdateSessionTimestamp(const std::string& session_id);
//获取某个会话的所有历史消息
std::vector<Message> GetSessionMessages(const std::string& session_id)const;

//获取所有会话列表：也就是获取所有的会话ID，之后用户再根据这些会话id获取自己想要的会话信息即可
std::vector<std::string> GetAllSessionIds()const;

//清空所有会话，包括会话ID、会话模型名称、会话消息列表、会话创建时间、会话修改时间等
void ClearAllSessions();

//获取会话总数
size_t GetSessionCount()const;

//删除指定会话
bool DeleteSession(const std::string& session_id);


private:
//构建会话id
std::string BuildSessionId()const;
//构建消息id
std::string BuildMessageId(size_t messageCount)const;
private:
    //管理会话，因为会话不用在意有序，因此选择效率更高的unordered_map
    std::unordered_map<std::string,std::shared_ptr<Session>> _sessions;//key=>会话id,value=>会话对象，通过会话id快速获取会话对象并进行管理，这里用指针将会话对象创建在堆上，可以节省空间
    mutable std::mutex _mutex;//互斥锁,防止并发访问
    //会话总数，须保证安全性
    static std::atomic<int64_t> _session_counter;
    
};

} // namespace ai_chat_sdk
