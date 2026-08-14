#pragma once
#include<spdlog/spdlog.h>
#include<mutex>
#include<memory>
namespace ai_chat_sdk
{
    //单例模式，确保只有一个实例存在
    class my_Logger
   {
    private:
    static std::shared_ptr<spdlog::logger> logger_; //声明一个静态的日志器对象
    //线程安全
    static std::mutex _mutex;//声明一个线程安全的互斥锁对象
    private:
    my_Logger();// = default;//默认构造函数
    my_Logger(const my_Logger&);// = default;//复制构造函数
    my_Logger& operator=(const my_Logger&);// = default;//赋值运算符
    public:
    static void Init(const std::string& logger_name,spdlog::level::level_enum level,std::string output_path);//初始化日志器 
    static std::shared_ptr<spdlog::logger> get_logger();//获取日志器对象
   };
}
   //因为spdlog库不支持输出错误信息的位置，因此定义宏来输出错误信息的位置
   #define ERR(format, ...) ai_chat_sdk::my_Logger::get_logger()->error(std::string("[{}:{}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
   #define INFO(format, ...) ai_chat_sdk::my_Logger::get_logger()->info(std::string("[{}:{}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)   
   #define DEBUG(format, ...) ai_chat_sdk::my_Logger::get_logger()->debug(std::string("[{}:{}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
   #define TRACE(format, ...) ai_chat_sdk::my_Logger::get_logger()->trace(std::string("[{}:{}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)  
   #define WARN(format, ...) ai_chat_sdk::my_Logger::get_logger()->warn(std::string("[{}:{}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)

