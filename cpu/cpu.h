#ifndef CPU_H
#define CPU_H

#include <map>
#include <functional> // Para std::function (nossos ISRs)
#include <string>
#include <iostream>
#include "../pic/ControladorPIC.h"

// 1. Depende da ABSTRAÇÃO, não mais do ControladorPIC.h
#include "../interface/IControladorIRQ.h"

class CPU
{
public:
    /**
     * @brief Construtor. A CPU aceita qualquer controlador
     * que siga o contrato IControladorIRQ.
     */
    explicit CPU(IControladorIRQ &controlador); // 2. Alterado

    /**
     * @brief Registra uma Rotina de Serviço de Interrupção (ISR)
     * na "Interrupt Descriptor Table" (IDT) desta CPU.
     *
     * @param linha O número da IRQ (ex: 1 para teclado).
     * @param isr A função (lambda, etc.) a ser executada.
     */
    void registrarISR(int linha, std::function<void()> isr);

    /**
     * @brief Executa um "tick" do clock da CPU.
     * Em um tick, a CPU primeiro verifica por interrupções.
     * Se houver uma, ela a executa.
     * Se não, ela faz um "trabalho fictício".
     */
    void tick();

private:
    // 3. O membro é uma REFERÊNCIA DE INTERFACE
    IControladorIRQ &m_controlador;

    // Nossa Tabela Descritora de Interrupções (IDT)
    // Mapeia uma linha de IRQ (int) a uma função (ISR)
    std::map<int, std::function<void()>> m_idt;

    void _fazerTrabalhoFicticio();
    void _log(const std::string &mensagem);
};

#endif // CPU_H