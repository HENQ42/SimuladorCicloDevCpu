#ifndef CONTROLADOR_PIC_H
#define CONTROLADOR_PIC_H

#include <map> // Para mapear: Linha IRQ -> Dispositivo
#include <string>
#include <iostream>
#include "../interface/IDispositivoIRQ.h" // Depende da ABSTRAÇÃO, não do teclado!
#include "../interface/IControladorIRQ.h"
/**
 * @class ControladorPIC
 * @brief Simula o Programmable Interrupt Controller (PIC).
 * * Responsabilidade: Monitora "canais" de IRQ ligados a dispositivos
 * (que implementam IDispositivoIRQ) e sinaliza a CPU quando um
 * canal fica ativo.
 */
class ControladorPIC : public IControladorIRQ
{
public:
    ControladorPIC();

    /**
     * @brief "Conecta" um dispositivo a um canal de IRQ específico.
     * (Simula a fiação da placa-mãe)
     *
     * @param linha O número da linha (ex: 1 para teclado, 12 para mouse)
     * @param dispositivo Um ponteiro para o dispositivo (que deve
     * implementar IDispositivoIRQ).
     */
    void registrarDispositivo(int linha, IDispositivoIRQ *dispositivo);

    /**
     * @brief Simula o "clock" do PIC, verificando todos os canais.
     * @return O número da linha da primeira interrupção ativa,
     * ou -1 se nenhuma estiver ativa.
     */
    int verificarInterrupcoes();

private:
    // Mapeia o canal (int) ao dispositivo (IDispositivoIRQ*)
    std::map<int, IDispositivoIRQ *> m_canaisIRQ;

    void _log(const std::string &mensagem);
};

#endif // CONTROLADOR_PIC_H