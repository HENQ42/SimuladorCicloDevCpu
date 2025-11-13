#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

// Nossas classes de simulação
#include "./cpu/cpu.h"
#include "./pic/ControladorPIC.h"
#include "./teclado/teclado.h"
#include "./buffer/BufferDeEntradaOS.h"

// Nossas implementações concretas
#include "./app/donut.h"
#include "./buffer/FileFrameBuffer.h"

// --- Constantes dos nossos arquivos de interface ---
const std::string ARQUIVO_LOGS = "sim_logs.txt";
const std::string ARQUIVO_FRAME = "sim_frame.txt";
const std::string ARQUIVO_INPUT = "sim_input.txt";

/**
 * @brief Função que a 'main' usa para fazer o papel do "socket".
 * Ela lê o arquivo de input, envia para o teclado e limpa o arquivo.
 */
void pollerDeInput(HardwareTeclado& teclado) {
    std::ifstream in(ARQUIVO_INPUT);
    if (!in.is_open()) return;

    std::string input;
    std::getline(in, input); // Lê a primeira linha
    in.close();

    if (!input.empty()) {
        // Envia o input para o hardware do teclado
        teclado.eventoUsuarioDigitou(input);
        
        // Limpa o arquivo de input
        std::ofstream out(ARQUIVO_INPUT, std::ios::trunc);
        out.close();
    }
}

int main() {
    // --- 1. Redirecionar Logs ---
    // Todo std::cout será escrito em 'sim_logs.txt'
    std::ofstream logStream(ARQUIVO_LOGS);
    std::streambuf* coutBuf = std::cout.rdbuf(); // Salva o buffer original
    std::cout.rdbuf(logStream.rdbuf()); // Redireciona

    std::cout << "--- SIMULADOR INICIADO ---" << std::endl;

    // --- 2. Criar Serviços, Hardware e Aplicação ---
    BufferDeEntradaOS bufferDeEntrada;
    FileFrameBuffer tela(ARQUIVO_FRAME); // Nossa interface de arquivo real
    
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

        // 4c. Simular a velocidade do clock (ex: 30 ticks por segundo)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // (Nunca vai chegar aqui, mas é boa prática)
    std::cout.rdbuf(coutBuf); // Restaura o stdout
    return 0;
}

