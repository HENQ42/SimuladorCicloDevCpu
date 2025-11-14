#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring> // Para memcpy

// --- DEPENDÊNCIAS POSIX PARA MMAP ---
#include <sys/mman.h> // mmap, munmap
#include <fcntl.h>    // open
#include <unistd.h>   // close, ftruncate
#include <sys/stat.h> // stat
// ------------------------------------

// Nossas classes de simulação
#include "./cpu/cpu.h"
#include "./pic/ControladorPIC.h"
#include "./teclado/teclado.h"
#include "./buffer/BufferDeEntradaOS.h"

// Nossas implementações concretas (vamos ignorar FileFrameBuffer.h)
#include "./app/donut.h"
#include "./interface/IFrameBuffer.h" // A interface que vamos implementar

// --- Constantes dos nossos arquivos de interface ---
const std::string ARQUIVO_LOGS = "sim_logs.txt";
const std::string ARQUIVO_FRAME = "sim_frame.txt";
const std::string ARQUIVO_INPUT = "sim_input.txt";

// Define o tamanho do buffer de frame. 
// W * H + H newlines (80 * 24 + 24) = 1944.
#define FRAME_BUFFER_SIZE (W * H + H) 

/**
 * @class MmapFrameBuffer
 * @brief Implementação de IFrameBuffer que usa memória mapeada
 * por arquivo (mmap) para alta performance IPC.
 */
class MmapFrameBuffer : public IFrameBuffer {
private:
    std::string m_caminhoArquivo;
    int m_fd;
    char* m_map_ptr;
    size_t m_size;

    void _log(const std::string& msg) {
        std::cout << "[MMAP FB] " << msg << std::endl;
    }

public:
    MmapFrameBuffer(const std::string& caminhoArquivo)
        : m_caminhoArquivo(caminhoArquivo), m_fd(-1), m_map_ptr(nullptr), m_size(FRAME_BUFFER_SIZE) {
        
        _log("Inicializando MmapFrameBuffer...");

        // 1. Abre/Cria o arquivo
        // O_CREAT: Cria se não existir. O_RDWR: Leitura e Escrita.
        m_fd = open(m_caminhoArquivo.c_str(), O_CREAT | O_RDWR, (mode_t)0600);
        if (m_fd == -1) {
            _log("ERRO: Falha ao abrir/criar o arquivo: " + m_caminhoArquivo);
            return;
        }

        // 2. Define o tamanho do arquivo (Crucial para MMAP)
        if (ftruncate(m_fd, m_size) == -1) {
            _log("ERRO: Falha ao definir o tamanho do arquivo com ftruncate.");
            close(m_fd);
            m_fd = -1;
            return;
        }

        // 3. Mapeia o arquivo para a memória
        m_map_ptr = (char*)mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (m_map_ptr == MAP_FAILED) {
            _log("ERRO: Falha ao mapear o arquivo para a memória com mmap.");
            close(m_fd);
            m_fd = -1;
            m_map_ptr = nullptr;
            return;
        }
        
        // Limpa o conteúdo inicial do buffer de memória (frame em branco)
        memset(m_map_ptr, ' ', m_size);
        _log("Mmap bem-sucedido. Framebuffer pronto.");
    }

    ~MmapFrameBuffer() override {
        if (m_map_ptr != nullptr) {
            munmap(m_map_ptr, m_size);
        }
        if (m_fd != -1) {
            close(m_fd);
        }
    }

    void limpar() override {
        // Limpar o buffer de memória com espaços
        if (m_map_ptr != nullptr) {
            memset(m_map_ptr, ' ', m_size);
            // msync para garantir a escrita imediata (opcional, mas bom para IPC)
            msync(m_map_ptr, m_size, MS_SYNC);
        }
    }

    void atualizar(const std::string& conteudo) override {
        if (m_map_ptr == nullptr || conteudo.empty()) {
            return;
        }
        
        // 1. Copia o conteúdo da string (o frame) para a memória mapeada
        size_t bytesToCopy = std::min(conteudo.size(), m_size);
        memcpy(m_map_ptr, conteudo.data(), bytesToCopy);
        
        // 2. Garante a sincronização imediata com o disco (opcional)
        msync(m_map_ptr, m_size, MS_SYNC);
    }
};


/**
 * @brief Função que a 'main' usa para fazer o papel do "socket".
 * Ela lê o arquivo de input, envia para o teclado e limpa o arquivo.
 */
void pollerDeInput(HardwareTeclado& teclado) {
    std::ifstream in(ARQUIVO_INPUT);
    if (!in.is_open()) return;

    std::string input;
    // Lê a primeira linha de input
    std::getline(in, input); 
    in.close();

    if (!input.empty()) {
        // Envia o input para o hardware do teclado
        teclado.eventoUsuarioDigitou(input);
        
        // Limpa o arquivo de input (truncando)
        std::ofstream out(ARQUIVO_INPUT, std::ios::trunc);
        out.close();
    }
}

int main() {
    // --- 0. CORREÇÃO: Criação inicial do arquivo de input ---
    // Isso garante que 'sim_input.txt' exista no sistema de arquivos,
    // corrigindo o bug onde o poller não conseguiria abri-lo.
    std::ofstream inputInit(ARQUIVO_INPUT, std::ios::out | std::ios::app);
    inputInit.close();

    // --- 1. Redirecionar Logs ---
    // Todo std::cout será escrito em 'sim_logs.txt'
    std::ofstream logStream(ARQUIVO_LOGS);
    std::streambuf* coutBuf = std::cout.rdbuf(); // Salva o buffer original
    std::cout.rdbuf(logStream.rdbuf()); // Redireciona

    std::cout << "--- SIMULADOR INICIADO (MMAP) ---" << std::endl;

    // --- 2. Criar Serviços, Hardware e Aplicação ---
    BufferDeEntradaOS bufferDeEntrada;
    // NOVO: Usando a implementação MMAP
    MmapFrameBuffer tela(ARQUIVO_FRAME); 
    
    HardwareTeclado teclado;
    ControladorPIC pic;
    CPU cpu(pic);
    
    AppDonut appDonut;

    // --- 3. Fazer a "Fiação" (SOLID) ---
    pic.registrarDispositivo(1, &teclado);

    cpu.registrarISR(1, [&teclado, &bufferDeEntrada]() {
        char c = (char)teclado.lerDados();
        bufferDeEntrada.enfileirarTecla(c);
        teclado.eventoCPULeuDados();
    });

    appDonut.conectar(&bufferDeEntrada, &tela);
    cpu.carregarAplicacao(&appDonut);

    std::cout << "Sistema montado. Iniciando loop principal..." << std::endl;

    // --- 4. Loop Principal (Infinito) ---
    // Este é o "clock" do nosso sistema
    while (true) {
        // 4a. Fazer o papel do "socket" (ler arquivo de input)
        pollerDeInput(teclado);
        
        // 4b. Executar um tick da CPU (que roda a AppDonut)
        cpu.tick();

        // NOVO: Flush forçado do log. Garante que os logs sejam escritos
        // no arquivo imediatamente para que o servidor WebSocket os leia.
        logStream.flush(); 

        // 4c. Simular a velocidade do clock (ex: 30 ticks por segundo)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // (Nunca vai chegar aqui)
    std::cout.rdbuf(coutBuf); // Restaura o stdout
    return 0;
}
