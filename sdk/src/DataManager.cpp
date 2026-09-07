#include"../include/DataManager.h"
#include"../include/util/my_spdlog.h"
namespace ai_chat_sdk 
{
    //构建数据库并且初始化数据库表
    DataManager::DataManager(const std::string& db_Name)
    {
        _db_Name = db_Name;
        auto result = sqlite3_open(db_Name.c_str(), &_db);
        if(result != SQLITE_OK)
        {
           ERR("数据库创建失败：{}",sqlite3_errmsg(_db));
        }
        //数据库创建成功，然后初始化数据库
        if(!InitTable())
        {
            ERR("数据库表初始化失败");
        }
    }
    //关闭数据库连接对象
    DataManager::~DataManager()
    {
        sqlite3_close(_db);
    }
    //初始化数据库表：Session表和Message表
    bool DataManager::InitTable()
    {
        //初始化Session表：
        //sql语句：同时与Message表关联起来作为外键
        std::string session_sql = R"(
        CREATE TABLE IF NOT EXISTS Session (
            session_id TEXT PRIMARY KEY,
            ModelName TEXT NOT NULL,
            create_time INTEGER NOT NULL,
            update_time INTEGER NOT NULL
        );
        )";
        if(!Execute(session_sql))
        {
            ERR("Session表初始化失败");
            return false;
        }

        //初始化Message表：
        //sql语句：
        std::string message_sql = R"(
        CREATE TABLE IF NOT EXISTS Message (
            message_id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            _content TEXT,
            _role TEXT,
            _create_time INTEGER DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (session_id) REFERENCES Session(session_id)
        );
        )";
        if(!Execute(message_sql))
        {
            ERR("Message表初始化失败");
            return false;
        }

        return true;
    }

    //执行sql语句
    bool DataManager::Execute(const std::string& sql)
    {
        auto result = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, nullptr);
        if(result != SQLITE_OK)
        {
            ERR("执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        return true;
    }

    //向数据库中插入一条会话
    bool DataManager::InsertSession(const Session& session)
    {
        //设置线程安全锁
        std::lock_guard<std::mutex> lock(_mutex);
        //sql语句
        std::string sql_sql = R"(
        INSERT INTO Session (session_id,ModelName,create_time,update_time)
        VALUES (?,?,?,?)
        )";
        //对sql语句进行语法检查
        sqlite3_stmt* stmt;//检查之后会生成一个stmt对象
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("InsertSession准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //绑定参数
        sqlite3_bind_text(stmt, 1, session.session_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, session.ModelName.c_str(), -1, SQLITE_STATIC);
        //为了更安全，将create_time和update_time转换为整数类型
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(session.create_time));
        sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(session.update_time));
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("InsertSession执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("InsertSession插入会话成功:{}",session.session_id);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }

    //获取数据库中的指定会话的信息
    std::shared_ptr<Session> DataManager::GetSession(const std::string& session_id)
    {
        //设置线程安全锁
        std::lock_guard<std::mutex> lock(_mutex);
        //sql语句
        std::string sql_sql = R"(
        SELECT session_id,ModelName,create_time,update_time FROM Session WHERE session_id=?
        )";
        //对sql语句进行语法检查
        sqlite3_stmt* stmt;//检查之后会生成一个stmt对象
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("GetSession准备sql语句失败:{}",sqlite3_errmsg(_db));
            return nullptr;
        }
        //绑定参数
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_ROW)
        {
            ERR("GetSession执行sql语句失败:{}",sqlite3_errmsg(_db));
            //释放stmt对象
            sqlite3_finalize(stmt);
            return nullptr;
        }
        //创建一个Session对象
        auto session = std::make_shared<Session>();
        //获取查询结果
        session->session_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        session->ModelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        session->create_time = sqlite3_column_int64(stmt, 2);
        session->update_time = sqlite3_column_int64(stmt, 3);

        //获取会话的消息列表
        session->messages = GetAllMessages(session_id);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return session;
    }

    //更新指定会话中的事件戳
    bool DataManager::UpdateSessionTime(const std::string& session_id,std::time_t update_time)
    {
        //sql语句
        std::string sql_sql = R"(
        UPDATE Session SET update_time=? WHERE session_id=?
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("UpdateSessionTime准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //绑定参数
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(update_time));
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_STATIC);
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("UpdateSessionTime执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("UpdateSessionTime更新会话时间戳成功:{}",update_time);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }

    //获取所有的会话id
    std::vector<std::string> DataManager::GetAllSessionIds()
    {
        //sql语句:以降序排列会话id，最近创建的会话在最前面
        std::string sql_sql = R"(
        SELECT session_id FROM Session ORDER BY create_time DESC    
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("GetAllSessionIds准备sql语句失败:{}",sqlite3_errmsg(_db));
            return {};
        }
        //检查没有问题之后执行语句
        std::vector<std::string> session_ids;
        while(sqlite3_step(stmt)==SQLITE_ROW)
        {
            session_ids.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        //释放stmt对象
        sqlite3_finalize(stmt);
        return session_ids;
    }
    //删除指定会话
    bool DataManager::DeleteSession(const std::string& session_id)
    {
        //sql语句
        std::string erase_sql = R"(
        DELETE FROM Session WHERE session_id=?
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, erase_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("DeleteSession准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //绑定参数
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("DeleteSession执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("DeleteSession删除会话成功:{}",session_id);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }
    
    std::vector<std::shared_ptr<Session>> DataManager::GetAllSessions()
    {
         std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string selectSQL = R"(
        SELECT session_id, ModelName, create_time, update_time FROM Session ORDER BY update_time DESC;
    )";

    // 准备SQL语句
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        ERR("getAllSessionIds - 准备语句失败：{}", sqlite3_errmsg(_db));
        return {};
    }

    std::vector<std::shared_ptr<Session>> sessions;
    while(sqlite3_step(stmt) == SQLITE_ROW){
        std::string sessionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string modelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int64_t createTime = sqlite3_column_int64(stmt, 2);
        int64_t updateTime = sqlite3_column_int64(stmt, 3);

        auto session = std::make_shared<Session>(modelName);
        session->session_id = sessionId;
        session->create_time = static_cast<std::time_t>(createTime);
        session->update_time = static_cast<std::time_t>(updateTime);
        sessions.push_back(session);

        // 历史消息暂时不获取，需要时再通过会话id来进行获取
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("getAllSessions - 获取所有会话信息成功, 会话总数：{}", sessions.size());
    return sessions;
    }





    //获取会话总数
    int DataManager::GetSessionCount()const
    {
        //sql语句
        std::string sql_sql = R"(
        SELECT COUNT(*) FROM Session
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("GetSessionCount准备sql语句失败:{}",sqlite3_errmsg(_db));
            return 0;
        }
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("GetSessionCount执行sql语句失败:{}",sqlite3_errmsg(_db));
            return 0;
        }
       //获取查询结果
        int count = sqlite3_column_int(stmt, 0);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return count;
    }





    //获取指定会话的消息列表
    std::vector<Message> DataManager::GetAllMessages(const std::string& session_id)const
    {
        //sql语句
        std::string message_sql = R"(
        SELECT message_id,_content,_role,_create_time FROM Message WHERE session_id=?
        )";
        //准备sql语句
        sqlite3_stmt* message_stmt;
        auto message_rs=sqlite3_prepare_v2(_db, message_sql.c_str(), -1, &message_stmt, nullptr);
        if(message_rs!=SQLITE_OK)
        {
            ERR("GetAllMessages准备sql语句失败:{}",sqlite3_errmsg(_db));
            return {};
        }
        //绑定参数
        sqlite3_bind_text(message_stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
        //检查没有问题之后执行语句
        std::vector<Message> messages;
        while(sqlite3_step(message_stmt)==SQLITE_ROW)
        {
            Message message;
            message._message_id = reinterpret_cast<const char*>(sqlite3_column_text(message_stmt, 0));
            message._content = reinterpret_cast<const char*>(sqlite3_column_text(message_stmt, 1));
            message._role = reinterpret_cast<const char*>(sqlite3_column_text(message_stmt, 2));
            message._create_time = sqlite3_column_int64(message_stmt, 3);
            messages.push_back(message);
        }
        //释放stmt对象
        sqlite3_finalize(message_stmt);
        return messages;
    }



    //向指定会话中插入信息
    bool DataManager::InsertMessage(const std::string& session_id,const Message& message)
    {
        //sql语句
        std::string insert_sql = R"(
        INSERT INTO Message (message_id,session_id,_content,_role,_create_time) VALUES (?,  ?,?,?,?)
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, insert_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("InsertMessage准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //绑定参数
        sqlite3_bind_text(stmt, 1, message._message_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, message._content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, message._role.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._create_time));
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("InsertMessage执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("InsertMessage插入消息成功:{}",message._message_id);
        //更新会话事件戳
        UpdateSessionTime(session_id, message._create_time);
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }


    //删除指定会话中的所有信息：注意：只删除的是会话中的消息，不会删除会话本身
    bool DataManager::DeleteMessages(const std::string& session_id)
    {
        //sql语句
        std::string delete_sql = R"(
        DELETE FROM Message WHERE session_id=?
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, delete_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("DeleteMessages准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //绑定参数
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("DeleteMessages执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("DeleteMessages删除会话消息成功:{}",session_id);
        //更新会话时间戳
        UpdateSessionTime(session_id, time(nullptr));
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }



    bool DataManager::DeleteAllSessions()//用来删除数据库中的所有会话信息
    {
        //sql语句
        std::string sql_sql = R"(
        DELETE FROM Session
        )";
        //准备sql语句
        sqlite3_stmt* stmt;
        auto rs=sqlite3_prepare_v2(_db, sql_sql.c_str(), -1, &stmt, nullptr);
        if(rs!=SQLITE_OK)
        {
            ERR("DeleteAllSessions准备sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        //检查没有问题之后执行语句
        auto result=sqlite3_step(stmt);
        if(result!=SQLITE_DONE)
        {
            ERR("DeleteAllSessions执行sql语句失败:{}",sqlite3_errmsg(_db));
            return false;
        }
        INFO("DeleteAllSessions删除所有会话成功");
        //释放stmt对象
        sqlite3_finalize(stmt);
        return true;
    }



}    
