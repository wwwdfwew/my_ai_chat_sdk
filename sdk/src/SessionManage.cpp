#include "../include/SessionManage.h"
#include "../include/util/my_spdlog.h"
#include <ctime>
#include <iomanip>
#include <sstream>
namespace ai_chat_sdk {
//构建会话id,我们采用session+时间戳+_session_counter的方式
std::string SessionManage::BuildSessionId() const {
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
std::string SessionManage::BuildMessageId(size_t messageCount) const {
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
  std::lock_guard<std::mutex> lock(_mutex);
  //我们是通过会话id管理会话的，因此创建会话要先构建会话id，然后将会话id和会话对象存储到unordered_map中
  std::string _session_id = BuildSessionId();
  //构建会话对象
  auto session = std::make_shared<Session>(model_name);
  session->session_id = _session_id;
  //将会话对象存储到unordered_map中
  _sessions[_session_id] = session;
  return _session_id;
}

//获取会话
std::shared_ptr<Session> SessionManage::GetSession(const std::string &session_id) const {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("Session not found:{}", session_id);
    return nullptr;
  }
  return it->second;
}

//向指定会话中添加消息
bool SessionManage::AddMessage(const std::string &session_id,
                               const Message &message) const {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("AddMessage failed,Session not found:{}", session_id);
    return false;
  }
  auto session = it->second;
  //创建一个消息：
  Message Mag;
  Mag._role=message._role;
  Mag._content=message._content;
  Mag._message_id=BuildMessageId(session->messages.size());//用size()获取会话中历史消息列表的大小
  session->messages.push_back(Mag);
  //更新会话修改时间
  session->update_time = std::time(nullptr);
  INFO("AddMessage success,session_id:{},session update time:{}",session_id,session->update_time);
  return true;
}

//添加消息之后更新时间戳
bool SessionManage::UpdateSessionTimestamp(const std::string &session_id) {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("UpdateSessionTimestamp failed,Session not found:{}", session_id);
    return false;
  }
  auto session = it->second;
  //更新会话修改时间
  session->update_time = std::time(nullptr);
  INFO("UpdateSessionTimestamp success,session_id:{},session update time:{}",session_id,session->update_time);
  return true;
}

//获取指定会话中的所有消息
std::vector<Message> SessionManage::GetSessionMessages(const std::string &session_id) const {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("GetSessionMessages failed,Session not found:{}", session_id);
    return {};
  }
  //返回该会话的历史消息列表
  return it->second->messages;
}
//获取所有会话id
std::vector<std::string> SessionManage::GetAllSessionIds() const {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //构建一个临时会话列表，按照时间戳进行降序排序
  std::vector<std::pair<std::time_t,std::shared_ptr<Session>>> session_list;
  session_list.reserve(_sessions.size());//提前开辟好空间
  for (auto &session : _sessions) {
    session_list.push_back({session.second->update_time,session.second});
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
  std::lock_guard<std::mutex> lock(_mutex);
  //清空unordered_map
  _sessions.clear();
}
//获取会话总数
size_t SessionManage::GetSessionCount() const {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //返回unordered_map的大小，即会话总数
  return _sessions.size();
}


//删除指定会话
bool SessionManage::DeleteSession(const std::string& session_id) {
  //设置线程安全锁
  std::lock_guard<std::mutex> lock(_mutex);
  //根据会话id获取会话对象
  auto it = _sessions.find(session_id);
  if (it == _sessions.end()) {
    ERR("DeleteSession failed,Session not found:{}", session_id);
    return false;
  }
  //删除会话
  _sessions.erase(it);
  INFO("DeleteSession success,session_id:{}",session_id);
  return true;
}


} // namespace ai_chat_sdk
