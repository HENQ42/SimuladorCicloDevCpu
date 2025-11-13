#include "cpu.h"
#include "../interface/IProcesso.h"

// 1. O construtor aceita a interface
CPU::CPU(IControladorIRQ &controlador) : m_controlador(controlador)
{
    _log("CPU inicializada e conectada ao Controlador de IRQ.");
}

void CPU::registrarISR(int linha, std::function<void()> isr)
{
    m_idt[linha] = isr;
    _log("ISR registrada na IDT para a linha IRQ " + std::to_string(linha) + ".");
}

void CPU::tick()
{
    // 2. Esta linha chama a função da interface.
    // Não faz ideia se é um PIC, APIC, etc. Perfeito!
    int linhaAtiva = m_controlador.verificarInterrupcoes();

    if (linhaAtiva != -1)
    {
        // --- Interrupção Detectada ---
        _log("Interrupção detectada! (IRQ " + std::to_string(linhaAtiva) + "). Pausando trabalho.");

        // 2. A CPU consulta a IDT para encontrar o driver
        if (m_idt.count(linhaAtiva))
        {
            // 3. Executa o ISR
            std::function<void()> isr = m_idt[linhaAtiva];
            _log("Despachando para ISR...");
            isr(); // Chama a função registrada
            _log("ISR concluído. Retomando...");
        }
        else
        {
            _log("AVISO: IRQ " + std::to_string(linhaAtiva) + " disparada, mas NENHUM ISR registrado (Kernel Panic!)");
        }
    }
    else
    {
        // --- Sem Interrupção ---
        // Em vez de "trabalho fictício", executa a aplicação
        if (m_aplicacaoAtual != nullptr)
        {
            m_aplicacaoAtual->executarTick(); // <-- MUDANÇA IMPORTANTE
        }
        else
        {
            // CPU ociosa, sem aplicação para rodar
        }
    }
}

void CPU::_log(const std::string &mensagem)
{
    std::cout << "[CPU] " << mensagem << std::endl;
}

/**
 * @brief Implementação do novo método
 */
void CPU::carregarAplicacao(IAplicacao *app)
{
    m_aplicacaoAtual = app;
    _log("Aplicação carregada na CPU.");
}