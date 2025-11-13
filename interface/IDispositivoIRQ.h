#ifndef I_DISPOSITIVO_IRQ_H
#define I_DISPOSITIVO_IRQ_H

/**
 * @class IDispositivoIRQ
 * @brief Interface (contrato) para qualquer hardware que possa
 * sinalizar uma linha de interrupção (IRQ).
 * * O ControladorPIC dependerá desta abstração, e não de
 * implementações concretas (como HardwareTeclado).
 * (Princípio da Inversão de Dependência - SOLID)
 */
class IDispositivoIRQ
{
public:
    // Destrutor virtual puro é necessário para interfaces
    virtual ~IDispositivoIRQ() = default;

    /**
     * @brief (Obrigatório) Pergunta ao dispositivo se ele está
     * ativamente sinalizando sua linha de IRQ.
     * @return true se o sinal está ATIVO, false caso contrário.
     */
    virtual bool estaSinalIRQAtivo() const = 0;
};

#endif // I_DISPOSITIVO_IRQ_H