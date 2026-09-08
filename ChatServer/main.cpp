#include "ChatServer.h"
#include <gflags/gflags.h>
#include <ai_chat_sdk/util/my_spdlog.h>
#include <fstream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

DEFINE_string(host, "0.0.0.0", "服务器绑定的地址");
DEFINE_int32(port, 8080, "服务器监听的端口号");
DEFINE_string(log_level, "INFO", "日志级别: TRACE, DEBUG, INFO, WARN, ERROR");
DEFINE_string(conf, "ChatServer.conf", "配置文件路径");
DEFINE_double(temperature, 0.7, "模型温度值，范围0~2");
DEFINE_int32(max_tokens, 2048, "模型生成的最大token数，不能为负数");

DEFINE_string(ollama_model_name, "deepseek-r1:1.5b", "ollama本地模型名称");
DEFINE_string(ollama_model_desc, "本地运行的Deepseek R1 1.5B模型", "ollama模型描述信息");
DEFINE_string(ollama_base_url, "http://localhost:11434", "ollama服务的基础URL");

static const std::string AIChatServer_VERSION = "1.0.0";

static std::atomic<bool> g_running(false);
static std::shared_ptr<ai_chat_server::ChatServer> g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        INFO("收到信号 {}, 正在停止服务器...", signal);
        g_running.store(false);
        if (g_server) {
            g_server->Stop();
        }
    }
}

spdlog::level::level_enum parse_log_level(const std::string& level_str) {
    std::string level = level_str;
    std::transform(level.begin(), level.end(), level.begin(), ::toupper);
    if (level == "TRACE") return spdlog::level::trace;
    if (level == "DEBUG") return spdlog::level::debug;
    if (level == "INFO") return spdlog::level::info;
    if (level == "WARN" || level == "WARNING") return spdlog::level::warn;
    if (level == "ERROR") return spdlog::level::err;
    return spdlog::level::info;
}

void print_version() {
    std::cout << "AIChatServer 版本: " << AIChatServer_VERSION << std::endl;
    std::cout << "基于 C++17 开发的 AI 聊天服务器" << std::endl;
}

void print_help() {
    std::cout << "========================================" << std::endl;
    std::cout << "         AIChatServer 使用帮助" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: ./AIChatServer [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "常用选项:" << std::endl;
    std::cout << "  -h, --help                显示此帮助信息" << std::endl;
    std::cout << "  -v, --version             显示版本号" << std::endl;
    std::cout << std::endl;
    std::cout << "服务器配置选项:" << std::endl;
    std::cout << "  --host=<string>           服务器绑定的地址 (默认: 0.0.0.0)" << std::endl;
    std::cout << "  --port=<int>              服务器监听的端口号 (默认: 8080)" << std::endl;
    std::cout << "  --log_level=<string>      日志级别: TRACE/DEBUG/INFO/WARN/ERROR (默认: INFO)" << std::endl;
    std::cout << "  --conf=<string>           配置文件路径 (默认: ChatServer.conf)" << std::endl;
    std::cout << std::endl;
    std::cout << "模型配置选项:" << std::endl;
    std::cout << "  --temperature=<double>    模型温度值，范围0~2 (默认: 0.7)" << std::endl;
    std::cout << "  --max_tokens=<int>        最大生成token数，不能为负 (默认: 2048)" << std::endl;
    std::cout << std::endl;
    std::cout << "Ollama本地模型配置:" << std::endl;
    std::cout << "  --ollama_model_name=<string>  ollama模型名称 (默认: deepseek-r1:1.5b)" << std::endl;
    std::cout << "  --ollama_model_desc=<string>  ollama模型描述信息" << std::endl;
    std::cout << "  --ollama_base_url=<string>    ollama服务基础URL (默认: http://localhost:11434)" << std::endl;
    std::cout << std::endl;
    std::cout << "注意: Deepseek 和 ChatGPT 的 API Key 从环境变量中读取:" << std::endl;
    std::cout << "  export deepseek_apikey=\"your_deepseek_key\"" << std::endl;
    std::cout << "  export chatgpt_apikey=\"your_chatgpt_key\"" << std::endl;
    std::cout << std::endl;
    std::cout << "使用案例:" << std::endl;
    std::cout << "  # 使用默认配置启动" << std::endl;
    std::cout << "  ./AIChatServer" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 指定地址和端口" << std::endl;
    std::cout << "  ./AIChatServer --host=127.0.0.1 --port=9090" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 使用配置文件启动" << std::endl;
    std::cout << "  ./AIChatServer --conf=./ChatServer.conf" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 设置模型参数" << std::endl;
    std::cout << "  ./AIChatServer --temperature=0.9 --max_tokens=4096" << std::endl;
    std::cout << std::endl;
    std::cout << "提供的HTTP API接口:" << std::endl;
    std::cout << "  GET    /api/models                  获取可用模型列表" << std::endl;
    std::cout << "  GET    /api/sessions                获取会话列表" << std::endl;
    std::cout << "  POST   /api/session                 创建新会话" << std::endl;
    std::cout << "  GET    /api/session/{id}/history    获取会话历史消息" << std::endl;
    std::cout << "  POST   /api/message                 发送消息(全量返回)" << std::endl;
    std::cout << "  POST   /api/message/async           发送消息(流式返回SSE)" << std::endl;
    std::cout << "  DELETE /api/session/{id}            删除指定会话" << std::endl;
    std::cout << "========================================" << std::endl;
}

void generate_default_config(const std::string& config_path) {
    std::ofstream ofs(config_path);
    if (!ofs.is_open()) {
        WARN("无法生成默认配置文件: {}", config_path);
        return;
    }
    ofs << "# AIChatServer 配置文件" << std::endl;
    ofs << "# 服务器配置" << std::endl;
    ofs << "--host=0.0.0.0" << std::endl;
    ofs << "--port=8080" << std::endl;
    ofs << "--log_level=INFO" << std::endl;
    ofs << std::endl;
    ofs << "# 模型配置" << std::endl;
    ofs << "--temperature=0.7" << std::endl;
    ofs << "--max_tokens=2048" << std::endl;
    ofs << std::endl;
    ofs << "# Ollama本地模型配置" << std::endl;
    ofs << "--ollama_model_name=deepseek-r1:1.5b" << std::endl;
    ofs << "--ollama_model_desc=本地运行的Deepseek R1 1.5B模型" << std::endl;
    ofs << "--ollama_base_url=http://localhost:11434" << std::endl;
    ofs.close();
    INFO("已生成默认配置文件: {}", config_path);
}

bool load_config_from_file(const std::string& config_path) {
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        INFO("配置文件不存在，将生成默认配置文件: {}", config_path);
        generate_default_config(config_path);
        return true;
    }

    std::vector<std::string> argvs;
    argvs.push_back("AIChatServer");

    std::string line;
    while (std::getline(ifs, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;

        size_t end = line.find_last_not_of(" \t\r\n");
        std::string param = line.substr(start, end - start + 1);
        if (param.empty()) continue;

        if (param.substr(0, 2) == "--") {
            argvs.push_back(param);
        }
    }
    ifs.close();

    std::vector<char*> c_args;
    for (auto& s : argvs) {
        c_args.push_back(const_cast<char*>(s.c_str()));
    }

    int argc = static_cast<int>(c_args.size());
    char** argv = c_args.data();

    gflags::ParseCommandLineFlags(&argc, &argv, false);
    INFO("已从配置文件加载参数: {}", config_path);
    return true;
}

bool validate_parameters(ai_chat_server::ChatServerConfig& config) {
    if (FLAGS_temperature < 0.0 || FLAGS_temperature > 2.0) {
        ERR("温度值 temperature 必须在 0~2 之间，当前值: {}", FLAGS_temperature);
        return false;
    }
    config.temperature = static_cast<float>(FLAGS_temperature);

    if (FLAGS_max_tokens < 0) {
        ERR("最大token数 max_tokens 不能为负数，当前值: {}", FLAGS_max_tokens);
        return false;
    }
    config.max_takens = FLAGS_max_tokens;

    char* deepseek_key = std::getenv("deepseek_apikey");
    char* chatgpt_key = std::getenv("chatgpt_apikey");

    bool has_cloud_key = false;
    if (deepseek_key && std::strlen(deepseek_key) > 0) {
        config.deepseekApiKey = deepseek_key;
        config.deepseekModelName = "deepseek-chat";
        has_cloud_key = true;
    }
    if (chatgpt_key && std::strlen(chatgpt_key) > 0) {
        config.gpt4oMiniApiKey = chatgpt_key;
        config.gpt4oMiniModelName = "gpt-4o-mini";
        has_cloud_key = true;
    }

    bool has_ollama = true;
    if (FLAGS_ollama_model_name.empty()) {
        ERR("ollama 模型名称 (ollama_model_name) 不能为空");
        has_ollama = false;
    }
    if (FLAGS_ollama_model_desc.empty()) {
        ERR("ollama 模型描述 (ollama_model_desc) 不能为空");
        has_ollama = false;
    }
    if (FLAGS_ollama_base_url.empty()) {
        ERR("ollama 基础URL (ollama_base_url) 不能为空");
        has_ollama = false;
    }

    if (has_ollama) {
        config.ollamaModelName = FLAGS_ollama_model_name;
        config.ollamaModelDesc = FLAGS_ollama_model_desc;
        config.ollamaBaseUrl = FLAGS_ollama_base_url;
    }

    if (!has_cloud_key && !has_ollama) {
        ERR("配置无效: 必须至少配置一个云端模型的API Key (deepseek_apikey 或 chatgpt_apikey)，或者正确配置 ollama 参数");
        return false;
    }

    config.host = FLAGS_host;
    config.port = FLAGS_port;

    return true;
}

int main(int argc, char* argv[]) {
    gflags::SetVersionString(AIChatServer_VERSION);
    gflags::SetUsageMessage("AIChatServer - AI聊天服务器\n使用 --help 查看帮助信息");

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        }
    }

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::level::level_enum log_level = parse_log_level(FLAGS_log_level);
    ai_chat_sdk::my_Logger::Init("AIChatServer", log_level, "stdout");

    if (!FLAGS_conf.empty()) {
        load_config_from_file(FLAGS_conf);
        gflags::ParseCommandLineFlags(&argc, &argv, true);
    }

    INFO("================================");
    INFO("AIChatServer 启动中...");
    INFO("版本: {}", AIChatServer_VERSION);
    INFO("================================");

    ai_chat_server::ChatServerConfig server_config;
    if (!validate_parameters(server_config)) {
        ERR("参数验证失败，服务器启动终止");
        return 1;
    }

    INFO("服务器配置: host={}, port={}", server_config.host, server_config.port);
    INFO("模型参数: temperature={}, max_tokens={}", server_config.temperature, server_config.max_takens);
    if (!server_config.deepseekModelName.empty()) {
        INFO("已启用云端模型: {}", server_config.deepseekModelName);
    }
    if (!server_config.gpt4oMiniModelName.empty()) {
        INFO("已启用云端模型: {}", server_config.gpt4oMiniModelName);
    }
    if (!server_config.ollamaModelName.empty()) {
        INFO("已启用本地模型: {} ({})", server_config.ollamaModelName, server_config.ollamaBaseUrl);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        g_server = std::make_shared<ai_chat_server::ChatServer>(server_config);
        if (!g_server->Start()) {
            ERR("服务器启动失败");
            return 1;
        }
        INFO("服务器启动成功，监听 http://{}:{}", server_config.host, server_config.port);
        g_running.store(true);

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (g_server && g_server->IsRunning()) {
            g_server->Stop();
        }
        INFO("服务器已安全退出");
    } catch (const std::exception& e) {
        ERR("服务器发生异常: {}", e.what());
        return 1;
    }

    return 0;
}
