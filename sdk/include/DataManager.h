//将会话存储到数据库中，我们这里的操作和对会话管理的操作是有一点类似的，只不过代码语句是对数据库的操作
#pragma once
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include "common.h"

namespace ai_chat_sdk {
class DataManager {
public:
//构造函数
DataManager(const std::string& db_Name);//用来创建数据库

//析构函数
~DataManager();//用来释放数据库权柄
//关于Session的操作
//向数据库中插入一条会话
bool InsertSession(const Session& session);//用来插入一条会话到数据库中
//获取数据库中的指定会话的信息
std::shared_ptr<Session> GetSession(const std::string& session_id);//根据会话id,来获取数据库中指定的会话信息
//更新数据库中的会话时间戳
bool UpdateSessionTime(const std::string& session_id,std::time_t update_time);//根据会话id,来更新数据库中的会话时间戳
//获取数据库中的所有会话信息
std::vector<std::shared_ptr<Session>> GetAllSessions();//用来查询数据库中的所有会话信息
//获取所有的会话id
std::vector<std::string> GetAllSessionIds();//用来查询数据库中的所有会话id
//删除指定会话：注意：要连同会话中的所有消息一同删除
bool DeleteSession(const std::string& session_id);//根据会话id,来删除数据库中的会话信息

//获取会话总数
int GetSessionCount()const;//用来查询数据库中的会话总数

//删除所有的会话
bool DeleteAllSessions();//用来删除数据库中的所有会话信息


/////////////////////////////////////
//Message的操作
//向指定会话中插入消息
bool InsertMessage(const std::string& session_id,const Message& message);//根据会话id,来插入一条消息到数据库中
//获取指定会话中的所有消息
std::vector<Message> GetAllMessages(const std::string& session_id)const;//根据会话id,来查询数据库中的所有消息
//删除指定会话中的所有消息：注意：只删除的是会话中的消息，不会删除会话本身
bool DeleteMessages(const std::string& session_id);//根据会话id,来删除数据库中的所有消息




private:
//初始化表
bool InitTable();
//执行语句
bool Execute(const std::string& sql);
private:
  sqlite3 *_db;
  std::string _db_Name;
  mutable std::mutex _mutex;
};
} // namespace ai_chat_sdk