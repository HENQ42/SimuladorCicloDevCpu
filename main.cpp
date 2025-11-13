#include <iostream>
#include <string>
#include <iomanip>    // Para std::hex, std::dec
#include <functional> // Para std::function

#include "./teclado/teclado.h"
#include "./pic/ControladorPIC.h" // (Assumindo que define ControladorPIC)
#include "./cpu/cpu.h"

/**
 * @brief Helper para "espiar" o estado atual do hardware do teclado
 */
void printTecladoStatus(const HardwareTeclado &teclado)
{
    uint8_t status = teclado.lerStatus();
    uint8_t dados = teclado.lerDados();
    bool irq = teclado.estaSinalIRQAtivo();

    std::cout << "  [TESTE] MMIO Status: 0x" << std::hex << (int)status << std::dec
              << " | MMIO Dados: 0x" << std::hex << (int)dados << std::dec;

    if (status == STATUS_DADOS_PRONTOS)
    {
        std::cout << " ('" << (char)dados << "')";
    }
    else
    {
        std::cout << " (' ')";
    }

    std::cout << " | Fio IRQ Ativo?: " << (irq ? "SIM" : "NAO") << std::endl;
}

// --- Teste Principal ---
int main()
{
    std::cout << "Iniciando Simulador Completo (CPU + PIC + Teclado)..." << std::endl;
    std::cout << "==================================================" << std::endl;

    // --- 1. Inicialização (Boot do "Sistema") ---
    // Cria os objetos concretos que representam o hardware
    HardwareTeclado teclado;
    ControladorPIC pic;

    // --- 2. "Fiação" (Injeção de Dependência) ---

    // 2a. Injeta o PIC (concreto) na CPU (que espera a interface IControladorIRQ)
    CPU cpu(pic);

    // 2b. Registra o hardware do teclado no canal 1 do PIC
    pic.registrarDispositivo(1, &teclado);
    std::cout << std::endl;

    // --- 3. "Instalação do Driver" ---
    // O "S.O." registra a Rotina de Serviço de Interrupção (ISR)
    // na IDT (Interrupt Descriptor Table) da CPU.
    std::cout << "--- Instalando Driver (Registrando ISR na CPU) ---" << std::endl;

    // Usamos um lambda que captura 'teclado' por referência.
    // Este lambda é o nosso "driver" (ISR)
    cpu.registrarISR(1, [&teclado]()
                     {
        // Este é o código do nosso "driver"
        std::cout << "  [ISR_DRIVER] IRQ 1 (Teclado) recebida! Lendo hardware..." << std::endl;
        
        if (teclado.lerStatus() == STATUS_DADOS_PRONTOS) {
            uint8_t dado = teclado.lerDados();
            std::cout << "  [ISR_DRIVER] Dado lido: 0x" << std::hex << (int)dado << std::dec << " ('" << (char)dado << "')" << std::endl;
            
            // "Reconhece" a leitura, o que limpa o status
            // e permite ao hardware enviar o próximo byte (se houver).
            teclado.eventoCPULeuDados();
        } else {
            std::cout << "  [ISR_DRIVER] AVISO: IRQ fantasma? Status do teclado está VAZIO." << std::endl;
        } });
    std::cout << std::endl;

    // --- 4. Início da Simulação (Teste "AB") ---
    std::cout << "--- CENARIO: Teste de Buffer (Digitacao Rapida 'AB') ---" << std::endl;

    // --- Tick 1 (Ocioso) ---
    std::cout << "\n--- Tick 1 (Sistema Ocioso) ---" << std::endl;
    cpu.tick();                  // CPU não vê IRQ, faz trabalho fictício
    printTecladoStatus(teclado); // Esperado: 0x0, 0x0, NAO

    // --- Evento: Usuário Digita "AB" ---
    std::cout << "\n--- Evento: Usuário digita 'AB' ---" << std::endl;
    teclado.eventoUsuarioDigitou("AB");
    // Hardware processa 'A', 'B' vai pro buffer. IRQ 1 fica ATIVA.
    printTecladoStatus(teclado); // Esperado: 0x1, 'A', SIM

    // --- Tick 2 (Processa 'A') ---
    std::cout << "\n--- Tick 2 (CPU detecta 'A') ---" << std::endl;
    cpu.tick();                  // CPU detecta IRQ 1, pausa, chama o ISR registrado.
                                 // O ISR lê 'A', o hardware puxa 'B' do buffer.
                                 // IRQ 1 fica ATIVA de novo.
    printTecladoStatus(teclado); // Esperado: 0x1, 'B', SIM

    // --- Tick 3 (Processa 'B') ---
    std::cout << "\n--- Tick 3 (CPU detecta 'B') ---" << std::endl;
    cpu.tick();                  // CPU detecta IRQ 1 de novo, chama o ISR.
                                 // O ISR lê 'B'. O buffer agora está vazio.
                                 // IRQ 1 fica INATIVA.
    printTecladoStatus(teclado); // Esperado: 0x0, 0x0, NAO

    // --- Tick 4 (Ocioso de novo) ---
    std::cout << "\n--- Tick 4 (Sistema Ocioso) ---" << std::endl;
    cpu.tick();                  // CPU não vê IRQ, faz trabalho fictício.
    printTecladoStatus(teclado); // Esperado: 0x0, 0x0, NAO

    std::cout << "\n==================================================" << std::endl;
    std::cout << "Todos os testes concluídos." << std::endl;

    return 0;
}
