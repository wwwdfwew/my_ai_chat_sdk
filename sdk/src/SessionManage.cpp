#include "../include/SessionManage.h"
#include "../include/util/my_spdlog.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include"../include/common.h"
namespace ai_chat_sdk {

SessionManage::SessionManage(const std::string& db_name)
:_dataManager(db_name)
{ 
  //项目演示中，未创建新会话时，将所有会话列表展示出来
  //获取所有的会话信息·
  auto sessions=_dataManager.GetAllSessions();
  //将所有会话信息添加到会话管理中
  for(auto& session:sessions)
  {
    _sessions[session->session_id]=session;
  }
}


//构建会话id,我们采用session+时间戳+_session_counter的方式
std::string SessionManage::BuildSessionId()  {
  //创建会话需要构建会话id，那么每次会话计数需要增加1
  //采用更安全的自增方式
  _session_counter.fetch_add(1);
  std::time_t _time = std::time(nullptr);
  //然后将这三个值拼接在一起，使用ostringstream,它可以通过<<格式化我们的字符串将其放到ostringstream对象中

  std::ostringstream oss;
  oss << "session" << _time << std::setw(8) << std::setfill('0')
      << _session_counter; //为了保持宽度统一，设置对齐宽度未8位，不够的用0填充左侧
  return oss.str();
}

//构建消息id
std::string SessionManage::BuildMessageId(size_t messageCount)  {
  //创建消息需要构建消息id，那么每次消息计数需要增加1
  //采用更安全的自增方式
  messageCount++;
  std::time_t _time = std::time(nullptr);
  //然后将这三个值拼接在一起，使用ostringstream,它可以通过<<格式化我们的字符串将其放到ostringstream对象中

  std::ostringstream oss;
  oss << "message" << _time << std::setw(8) << std::setfill('0')
      << messageCount; //为了保持宽度统一，设置对齐宽度未8位，不够的用0填充左侧
  return oss.str();
}

//创建会话
std::string SessionManage::CreateSession(const std::string &model_name) {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  _mutex.lock();
  //我们是通过会话id管理会话的，因此创建会话要先构建会话id，然后将会话id和会话对象存储到unordered_map中
  std::string _session_id = BuildSessionId();
  //构建会话对象
  auto session = std::make_shared<Session>(model_name);
  session->session_id = _session_id;
  session->ModelName = model_name;
  session->create_time = std::time(nullptr);
  session->update_time = std::time(nullptr);
  //将会话对象存储到unordered_map中
  _sessions[_session_id] = session;
  //解锁
  _mutex.unlock();
  //将会话对象持久化存储到文件中
  _dataManager.InsertSession(*session);//因为会话管理的方法中也设置了锁，这样会造成死锁，因此我们手动给创建会话添加锁
  return _session_id;
}

//获取会话
std::shared_ptr<Session> SessionManage::GetSession(const std::string &session_id) {
  // //设置线程安全锁
  // //std::lock_guard<std::mutex> lock(_mutex);
  // _mutex.lock();
  // //根据会话id获取会话对象
  // auto it = _sessions.find(session_id);
  // if (it == _sessions.end()) {
  //   ERR("Session not found:{}", session_id);
  //   //解锁
  //   _mutex.unlock();
  //   //直接从会话管理中没能获取到会话，再从数据库中获取会话
  //   auto session = _dataManager.GetSession(session_id);
  //   if (session == nullptr) {
  //     ERR("GetSession failed,Session not found:{}", session_id);
  //     return nullptr;
  //   }
  //   //将数据库中的会话对象存储到unordered_map中
  //   _sessions[session_id] = session;
  // }
  // //解锁
  // _mutex.unlock();
  // //如果可以从会话中找到，就直接返回会话对象
  // return it->second;


  _mutex.lock();
    auto it = _sessions.find(session_id);
    if (it != _sessions.end()) {
        _mutex.unlock();
        return it->second;
    }
    _mutex.unlock();

    // 从数据库加载
    auto session = _dataManager.GetSession(session_id);
    if (session == nullptr) {
        ERR("GetSession failed, session not found: {}", session_id);
        return nullptr;
    }
    _mutex.lock();
    _sessions[session_id] = session;
    _mutex.unlock();
    return session;  // 直接返回新加载的会话，避免使用失效迭代器
}

//向指定会话中添加消息
bool SessionManage::AddMessage(const std::string &session_id,
                               const Message &message) {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  _mutex.lock();
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("AddMessage failed,Session not found in session:{}", session_id);
    //解锁
    _mutex.unlock();
    //从会话中找不到会话，再从数据库中查找会话
    auto session = _dataManager.GetSession(session_id);
    if (session == nullptr) {
      ERR("AddMessage failed,Session not found in database:{}", session_id);
      return false;
    }
    //查找到了，
    _sessions[session_id] = session;
  }
  //解锁
  _mutex.unlock();
  it = _sessions.find(session_id);
  auto session = it->second;

  //创建一个消息：
  Message Mag;
  Mag._role=message._role;
  Mag._content=message._content;
  Mag._message_id=BuildMessageId(session->messages.size());//用size()获取会话中历史消息列表的大小
  session->messages.push_back(Mag);
  //将消息存储到数据库中
  _dataManager.InsertMessage(session_id,Mag);
  //更新会话修改时间
  session->update_time = std::time(nullptr);
  //更新数据库中的时间
  _dataManager.UpdateSessionTime(session_id,session->update_time);
  INFO("AddMessage success,session_id:{},session update time:{}",session_id,session->update_time);
  return true;

  // std::lock_guard<std::mutex> lock(_mutex);
  //   auto it = _sessions.find(session_id);
  //   std::shared_ptr<Session> session;
  //   if (it != _sessions.end()) {
  //       session = it->second;
  //   } else {
  //       // 不能直接在锁内调用 _dataManager.GetSession，因为它可能也加锁导致死锁
  //       // 需要先解锁再查询，但为了简化，我们暂时认为不会死锁，或者使用递归锁
  //       // 为安全起见，这里先解锁再加载（但可能造成不一致，不过本次修复主要解决崩溃）
  //       _mutex.unlock();
  //       session = _dataManager.GetSession(session_id);
  //       _mutex.lock();
  //       if (!session) {
  //           ERR("AddMessage failed, session not found");
  //           return false;
  //       }
  //       _sessions[session_id] = session;
  //   }
  //   // 此时 session 非空
  //   Message Mag = message;
  //   Mag._message_id = BuildMessageId(session->messages.size());
  //   session->messages.push_back(Mag);
  //   _dataManager.InsertMessage(session_id, Mag);
  //   session->update_time = std::time(nullptr);
  //   _dataManager.UpdateSessionTime(session_id, session->update_time);
  //   return true;
}

//添加消息之后更新时间戳
bool SessionManage::UpdateSessionTimestamp(const std::string &session_id) {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  _mutex.lock();
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("UpdateSessionTimestamp failed,Session not found in session:{}", session_id);
    //解锁
    _mutex.unlock();
    //在数据库中查找会话
    auto session = _dataManager.GetSession(session_id);
    if (session == nullptr) {
      ERR("UpdateSessionTimestamp failed,Session not found in database:{}", session_id);
      return false;
    }
    //查找到了，
    _sessions[session_id] = session;

  }
  //解锁
  _mutex.unlock();
  auto session = it->second;
  //更新会话修改时间
  session->update_time = std::time(nullptr);
  //更新数据库中的时间
  _dataManager.UpdateSessionTime(session_id,session->update_time);
  INFO("UpdateSessionTimestamp success,session_id:{},session update time:{}",session_id,session->update_time);
  return true;

    // _mutex.lock();
    // auto it = _sessions.find(session_id);
    // std::shared_ptr<Session> session;
    // if (it != _sessions.end()) {
    //     session = it->second;
    //     _mutex.unlock();
    // } else {
    //     _mutex.unlock();
    //     session = _dataManager.GetSession(session_id);
    //     if (!session) {
    //         ERR("UpdateSessionTimestamp failed, session not found");
    //         return false;
    //     }
    //     _mutex.lock();
    //     _sessions[session_id] = session;
    //     _mutex.unlock();
    // }
    // if (!session) return false;
    // session->update_time = std::time(nullptr);
    // _dataManager.UpdateSessionTime(session_id, session->update_time);
    // INFO("UpdateSessionTimestamp success, session_id: {}", session_id);
    // return true;
// 更新 session->update_time
}

//获取指定会话中的所有消息
std::vector<Message> SessionManage::GetSessionMessages(const std::string &session_id)  {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  _mutex.lock();
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("GetSessionMessages failed,Session not found in session:{}", session_id);
    //解锁
    _mutex.unlock();
    //在数据库中查找会话
    auto session = _dataManager.GetSession(session_id);
    if (session == nullptr) {
      ERR("GetSessionMessages failed,Session not found in database:{}", session_id);
      return {};
    }
    //在数据库中查找到了会话，就将会话对象存储到unordered_map中，同时将数据库中的消息列表移动到会话对象中
    //将数据库中的消息列表移动到会话对象中
    _sessions[session_id] = _dataManager.GetSession(session_id);
    _sessions[session_id]->messages=_dataManager.GetAllMessages(session_id);
  }
  //解锁
  _mutex.unlock();
  return it->second->messages;

  // // 先从内存中获取会话消息，如果内存中获取不到，再到数据库中获取
  //   _mutex.lock();
  //   auto it = _sessions.find(session_id);
  //   if(it != _sessions.end()){
  //       _mutex.unlock();
  //       return it->second->messages;
  //   }
  //   _mutex.unlock();

  //   // 从数据库中获取消息列表
  //   return _dataManager.GetAllMessages(session_id);
}


//获取所有会话id
std::vector<std::string> SessionManage::GetAllSessionIds()  {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  _mutex.lock();
  //构建一个临时会话列表，按照时间戳进行降序排序
  std::vector<std::pair<std::time_t,std::shared_ptr<Session>>> session_list;
  //解锁
  _mutex.unlock();
  auto sessions=_dataManager.GetAllSessions();
  session_list.reserve(sessions.size());//提前开辟好空间
  for (auto &session : sessions) {
    session_list.push_back({session->update_time,session});
  }
  //根据时间戳降序排序
  std::sort(session_list.begin(),session_list.end(),[](const std::pair<std::time_t,std::shared_ptr<Session>>& a,const std::pair<std::time_t,std::shared_ptr<Session>>& b)
  {
    return a.first>b.first;
  });
  //将排序后的会话列表转换为vector
  std::vector<std::string> session_ids;
  for(int i=0;i<session_list.size();i++)
  {
    session_ids.push_back(session_list[i].second->session_id);
  }
  return session_ids;
}

//清空所有会话，包括会话ID、会话模型名称、会话消息列表、会话创建时间、会话修改时间等
void SessionManage::ClearAllSessions() {
  //设置线程安全锁
  _mutex.lock();
  //清空unordered_map
  _sessions.clear();
  _mutex.unlock();
  //清空数据库中的所有会话
  _dataManager.DeleteAllSessions();
  
}
//获取会话总数
size_t SessionManage::GetSessionCount()  {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  //解锁
  _mutex.unlock();
  //从数据库中获取所有会话
  auto sessions=_dataManager.GetAllSessions();
  //将数据库中的会话总数赋值给会话管理中的会话总数
  _session_counter=sessions.size();
  //返回unordered_map的大小，即会话总数
  return _session_counter;
}


//删除指定会话
bool SessionManage::DeleteSession(const std::string& session_id) {
  //设置线程安全锁
  //std::lock_guard<std::mutex> lock(_mutex);
  //解锁
  _mutex.unlock();
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("DeleteSession failed,Session not found:{}", session_id);
    //解锁
    _mutex.unlock();
    //在数据库中查找会话
    auto session = _dataManager.GetSession(session_id);
    if (session == nullptr) {
      ERR("DeleteSession failed,Session not found in database:{}", session_id);
      return false;
    }
  }
  //删除会话
  if(it!=_sessions.end())
  {
    //从unordered_map中删除会话
    _sessions.erase(it);
  }
  //解锁
  _mutex.unlock();
  //删除数据库中的会话
  _dataManager.DeleteSession(session_id);
  //更新会话修改时间
  _dataManager.UpdateSessionTime(session_id,time(nullptr));
  INFO("DeleteSession success,session_id:{}",session_id);
  return true;
}


} // namespace ai_chat_sdk
