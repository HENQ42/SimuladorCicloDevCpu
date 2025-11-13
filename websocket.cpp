#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <set>
#include <memory>
#include <mutex>

// --- Bibliotecas WebSocket ---
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// --- Configuração de Tipos WebSocket ---
typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::connection_hdl connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// --- Nossas interfaces de arquivo ---
const std::string ARQUIVO_LOGS = "sim_logs.txt";
const std::string ARQUIVO_FRAME = "sim_frame.txt";
const std::string ARQUIVO_INPUT = "sim_input.txt";

// ---
// --- BLOCO 1: O "BROADCASTER"
// ---

class WebSocketBroadcaster {
public:
    void addClient(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.insert(hdl);
    }

    void removeClient(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.erase(hdl);
    }

    void broadcast(const std::string& jsonMessage) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_server) return;
        for (auto& hdl : m_connections) {
            // Tenta enviar. Se falhar, o cliente será removido no 'close_handler'
            websocketpp::lib::error_code ec;
            m_server->send(hdl, jsonMessage, websocketpp::frame::opcode::text, ec);
        }
    }
    
    void setServer(server* s) { m_server = s; }

private:
    std::mutex m_mutex;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
    server* m_server = nullptr;
};

// ---
// --- BLOCO 2: Lógica do Intermediador (O "Poller" de Arquivos)
// ---

WebSocketBroadcaster g_broadcaster;
server g_ws_server;
std::string g_ultimoFrame = "";
std::string g_ultimosLogs = "";
std::mutex g_file_mutex;

std::string lerArquivo(const std::string& caminho) {
    std::ifstream in(caminho);
    if (!in.is_open()) return "";
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string formatarJson(const std::string& type, const std::string& data) {
    std::string json = "{\"type\": \"" + type + "\", \"data\": \"";
    for (char c : data) {
        if (c == '\"') json += "\\\"";
        else if (c == '\\') json += "\\\\";
        else if (c == '\n') json += "\\n";
        else json += c;
    }
    json += "\"}";
    return json;
}

/**
 * @brief Remove o frame do donut dos logs, se estiver presente.
 * O frame sempre começa com a sequência de escape \x1b[H.
 */
std::string limparDonutDosLogs(const std::string& logs) {
    std::string cleanLogs = "";
    std::stringstream ss(logs);
    std::string line;
    const std::string donutStart = "\x1b[H";

    while (std::getline(ss, line)) {
        // Se a linha começar com a sequência de escape do donut, ignore.
        // Isso remove o frame completo do log.
        if (line.find(donutStart) == 0) {
            // Se encontrar a sequência, pula esta "seção" (que é o frame)
            // e continua procurando o próximo log.
            continue; 
        }
        cleanLogs += line + "\n";
    }

    // Remove a última quebra de linha extra
    if (!cleanLogs.empty() && cleanLogs.back() == '\n') {
        cleanLogs.pop_back();
    }
    
    return cleanLogs;
}


void threadPoller() {
    while (true) {
        std::string frameConteudo;
        std::string logConteudo;
        std::string logConteudoLimpo;

        {
            std::lock_guard<std::mutex> lock(g_file_mutex);
            frameConteudo = lerArquivo(ARQUIVO_FRAME);
            logConteudo = lerArquivo(ARQUIVO_LOGS);
        }

        // Limpeza: Remove o frame do donut que está vazando para o log
        logConteudoLimpo = limparDonutDosLogs(logConteudo);

        // 1. Envia o Frame (se mudou)
        if (frameConteudo != g_ultimoFrame) {
            g_ultimoFrame = frameConteudo;
            g_broadcaster.broadcast(formatarJson("frame", frameConteudo));
        }

        // 2. Envia os Logs Limpos (se mudou)
        if (logConteudoLimpo != g_ultimosLogs) {
            g_ultimosLogs = logConteudoLimpo;
            g_broadcaster.broadcast(formatarJson("log", logConteudoLimpo));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

// Chamado quando o front-end envia uma mensagem
void onMessage(connection_hdl hdl, server::message_ptr msg) {
    std::string payload = msg->get_payload();

    // "Parse" de JSON
    if (payload.find("\"type\": \"input\"") != std::string::npos) {
        std::string tecla;
        if (payload.find("\"key\": \"w\"") != std::string::npos) tecla = "w";
        else if (payload.find("\"key\": \"a\"") != std::string::npos) tecla = "a";
        else if (payload.find("\"key\": \"s\"") != std::string::npos) tecla = "s";
        else if (payload.find("\"key\": \"d\"") != std::string::npos) tecla = "d";

        if (!tecla.empty()) {
            std::cout << "[Intermediador] Recebido input: " << tecla << ". Escrevendo em " << ARQUIVO_INPUT << std::endl;
            std::ofstream out(ARQUIVO_INPUT, std::ios::trunc);
            out << tecla;
            out.close();
        }
    }
    
    // Comando para limpar o arquivo de log
    if (payload.find("\"type\": \"clear_logs\"") != std::string::npos) {
        std::cout << "[Intermediador] Recebido pedido para limpar logs." << std::endl;
        {
            std::lock_guard<std::mutex> lock(g_file_mutex);
            std::ofstream out(ARQUIVO_LOGS, std::ios::trunc);
            out.close();
            g_ultimosLogs = "";
        }
    }
}


int main() {
    std::cout << "--- Intermediador WebSocket-Arquivo Iniciado ---" << std::endl;
    
    g_broadcaster.setServer(&g_ws_server);

    // Silencia os logs de debug do websocketpp
    g_ws_server.clear_access_channels(websocketpp::log::alevel::all);

    g_ws_server.init_asio();
    g_ws_server.set_reuse_addr(true);

    // Callbacks
    g_ws_server.set_open_handler(bind(&WebSocketBroadcaster::addClient, &g_broadcaster, ::_1));
    g_ws_server.set_close_handler(bind(&WebSocketBroadcaster::removeClient, &g_broadcaster, ::_1));
    g_ws_server.set_message_handler(bind(&onMessage, ::_1, ::_2));

    // --- Iniciar Threads ---
    
    std::thread ws_thread([]() {
        g_ws_server.listen(8065); // Porta 8065 conforme solicitado
        g_ws_server.start_accept();
        g_ws_server.run();
    });

    std::thread file_poller_thread(threadPoller);

    std::cout << "Servidor escutando na porta 8065." << std::endl;
    
    ws_thread.join();
    file_poller_thread.join();
    
    return 0;
}


