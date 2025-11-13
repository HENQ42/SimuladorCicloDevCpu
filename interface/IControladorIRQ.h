#ifndef I_CONTROLADOR_IRQ_H
#define I_CONTROLADOR_IRQ_H

/**
 * @class IControladorIRQ
 * @brief Interface (contrato) para qualquer hardware que
 * gerencia e sinaliza interrupções para a CPU.
 * (Abstração para o Princípio da Inversão de Dependência)
 */
class IControladorIRQ
{
public:
    virtual ~IControladorIRQ() = default;

    /**
     * @brief (Obrigatório) Pergunta ao controlador qual linha
     * de IRQ está ativa.
     * @return O número da linha ativa, ou -1 se nenhuma.
     */
    virtual int verificarInterrupcoes() = 0;
};

#endif // I_CONTROLADOR_IRQ_H