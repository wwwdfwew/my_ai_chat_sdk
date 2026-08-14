#include"../../include/util/my_spdlog.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include<spdlog/async.h>


namespace ai_chat_sdk
{
    //定义日志器对象
    std::shared_ptr<spdlog::logger> my_Logger::logger_;
    //定义线程安全的互斥锁对象
    std::mutex my_Logger:: _mutex;

    std::shared_ptr<spdlog::logger> my_Logger::get_logger()
    {
        return logger_;
    }

     void my_Logger::Init(const std::string& logger_name,spdlog::level::level_enum level,std::string output_path)
     {
        //异步日志，先判断是否已经初始化
        if(logger_==nullptr)
        {
            //异步日志需要异步线程池的支持，所以异步线程池初始化，线程数为1，缓冲区大小为2048
            spdlog::init_thread_pool(2048,1);
            if(output_path=="stdout")//标准错误输出
            {
                //使用日志器记录工程初始化日志器对象
                logger_ = spdlog::stdout_color_mt(logger_name);
            }
            else//输出到指定文件当中
            {
                //使用日志器记录工程初始化日志器对象
                logger_=spdlog::basic_logger_mt<spdlog::async_factory>(logger_name,output_path);
            }
            //设置日志等级：
            logger_->set_level(level);
            //设置日志格式：
            logger_->set_pattern("%H:%M:%S [%t] [%-7l] %v");
        }
     }
}