#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <set>
#include <memory>
#include <mutex>

// --- Bibliotecas WebSocket ---
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// --- Nossas Classes de Simulação (Não mudam nada) ---
#include "./cpu/cpu.h"
#include "./pic/ControladorPIC.h"
#include "./teclado/teclado.h"
#include "./buffer/BufferDeEntradaOS.h"
#include "./app/donut.h"
#include "./interface/IFrameBuffer.h" // Precisamos da interface

// --- Configuração de Tipos WebSocket ---
typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::connection_hdl connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// ---
// --- BLOCO 1: O "BROADCASTER" (A Ponte de Saída)
// ---
// Esta classe gerencia todas as conexões web e envia dados para elas.
// Ela é thread-safe.
// ---

class WebSocketBroadcaster {
public:
    // Adiciona uma nova conexão de cliente
    void addClient(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.insert(hdl);
    }

    // Remove um cliente que desconectou
    void removeClient(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.erase(hdl);
    }

    // Envia uma string JSON para todos os clientes conectados
    void broadcast(const std::string& jsonMessage) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& hdl : m_connections) {
            m_server->send(hdl, jsonMessage, websocketpp::frame::opcode::text);
        }
    }
    
    // (Precisamos dar a ela um ponteiro para o servidor depois que ele for criado)
    void setServer(server* s) { m_server = s; }

private:
    std::mutex m_mutex;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
    server* m_server = nullptr;
};

// ---
// --- BLOCO 2: AS NOVAS IMPLEMENTAÇÕES DE INTERFACE (A Ponte)
// ---

/**
 * @class WebSocketFrameBuffer
 * @brief Implementação do IFrameBuffer que envia o frame pela web.
 */
class WebSocketFrameBuffer : public IFrameBuffer {
public:
    WebSocketFrameBuffer(WebSocketBroadcaster& broadcaster) : m_broadcaster(broadcaster) {}

    void limpar() override { /* Não é necessário, o \x1b[H cuida disso */ }
    
    void atualizar(const std::string& conteudo) override {
        // Formata como o JSON que o front-end espera
        std::string json = "{\"type\": \"frame\", \"data\": \"";
        
        // (Escapa caracteres especiais para JSON, como \n e ")
        for (char c : conteudo) {
            if (c == '\"') json += "\\\"";
            else if (c == '\\') json += "\\\\";
            else if (c == '\n') json += "\\n";
            else json += c;
        }
        json += "\"}";
        
        m_broadcaster.broadcast(json);
    }

private:
    WebSocketBroadcaster& m_broadcaster;
};

/**
 * @class WebSocketLogRedirect
 * @brief Um streambuf customizado que redireciona std::cout
 * para o nosso broadcaster WebSocket.
 */
class WebSocketLogRedirect : public std::streambuf {
public:
    WebSocketLogRedirect(WebSocketBroadcaster& broadcaster) : m_broadcaster(broadcaster) {}

protected:
    // Chamado quando o buffer (p) está cheio ou em std::endl
    virtual int_type overflow(int_type c = EOF) override {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
        }
        return c;
    }

    virtual int sync() override {
        if (!m_buffer.empty()) {
            // Formata como o JSON que o front-end espera
            std::string json = "{\"type\": \"log\", \"data\": \"";
            // (Aqui também precisaríamos escapar, mas vamos simplificar)
            json += m_buffer;
            json += "\"}";
            
            m_broadcaster.broadcast(json);
            m_buffer.clear();
        }
        return 0; // Sucesso
    }

private:
    WebSocketBroadcaster& m_broadcaster;
    std::string m_buffer;
};

// ---
// --- BLOCO 3: A NOVA "MAIN" (O SERVIDOR)
// ---

int main() {
    // --- 1. Criar os Objetos Globais da Ponte ---
    server ws_server;
    ws_server.clear_access_channels(websocketpp::log::alevel::all);
    ws_server.clear_error_channels(websocketpp::log::elevel::all);
    
    WebSocketBroadcaster broadcaster;
    broadcaster.setServer(&ws_server); // Dá ao broadcaster o controle do servidor

    // --- 2. Redirecionar Logs ---
    WebSocketLogRedirect logRedirect(broadcaster);
    std::streambuf* oldCout = std::cout.rdbuf(); // Salva o cout antigo
    std::cout.rdbuf(&logRedirect); // Redireciona!
    std::cout.rdbuf(oldCout); // Restaura (só para o log inicial)
    
    std::cout << "--- SIMULADOR INICIADO (Modo WebSocket) ---" << std::endl;
    std::cout.rdbuf(&logRedirect); // Redireciona DE VERDADE agora

    // --- 3. Criar Serviços, Hardware e Aplicação ---
    // (Exatamente como antes)
    BufferDeEntradaOS bufferDeEntrada;
    WebSocketFrameBuffer tela(broadcaster); // <--- USA A NOVA INTERFACE!
    
    HardwareTeclado teclado; // Já é thread-safe
    ControladorPIC pic;
    CPU cpu(pic);
    
    AppDonut appDonut;

    // --- 4. Fazer a "Fiação" (SOLID) ---
    // (Exatamente como antes)
    pic.registrarDispositivo(1, &teclado);
    cpu.registrarISR(1, [&teclado, &bufferDeEntrada]() {
        char c = (char)teclado.lerDados();
        bufferDeEntrada.enfileirarTecla(c);
        teclado.eventoCPULeuDados();
    });
    appDonut.conectar(&bufferDeEntrada, &tela);
    cpu.carregarAplicacao(&appDonut);
    
    std::cout << "Sistema montado. Iniciando loop..." << std::endl;

    // --- 5. Configurar o Servidor WebSocket ---
    ws_server.init_asio();
    ws_server.set_reuse_addr(true); // Permite reusar o endereço

    // Callbacks do WebSocket
    ws_server.set_open_handler(bind(&WebSocketBroadcaster::addClient, &broadcaster, ::_1));
    ws_server.set_close_handler(bind(&WebSocketBroadcaster::removeClient, &broadcaster, ::_1));
    
    // Callback de Mensagem (A Ponte de Entrada)
    ws_server.set_message_handler(
        [&teclado](connection_hdl hdl, server::message_ptr msg) {
        
        std::string payload = msg->get_payload();
        std::cout << "[WebSocket] Mensagem recebida: " << payload << std::endl;

        // "Parse" de JSON (simples)
        if (payload.find("\"type\": \"input\"") != std::string::npos) {
            if (payload.find("\"key\": \"w\"") != std::string::npos) teclado.eventoUsuarioDigitou("w");
            if (payload.find("\"key\": \"a\"") != std::string::npos) teclado.eventoUsuarioDigitou("a");
            if (payload.find("\"key\": \"s\"") != std::string::npos) teclado.eventoUsuarioDigitou("s");
            if (payload.find("\"key\": \"d\"") != std::string::npos) teclado.eventoUsuarioDigitou("d");
        }
        if (payload.find("\"type\": \"clear_logs\"") != std::string::npos) {
            std::cout << "[WebSocket] Pedido de limpeza de logs (implementação futura)..." << std::endl;
        }
    });

    // --- 6. Iniciar Threads ---
    
    // Thread A: Servidor WebSocket
    std::thread ws_thread([&ws_server]() {
        ws_server.listen(8065); // Escuta na porta 8080
        ws_server.start_accept();
        ws_server.run(); // Bloqueia esta thread
        std::cout << "Servidor WebSocket parado." << std::endl;
    });

    // Thread B: Simulação da CPU
    std::thread sim_thread([&cpu]() {
        std::cout << "Thread da CPU iniciada." << std::endl;
        while (true) {
            cpu.tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        }
    });

    // --- 7. Esperar Threads Terminarem ---
    ws_thread.join();
    sim_thread.join();
    
    std::cout.rdbuf(oldCout); // Restaura o cout
    return 0;
}

