#ifndef HARDWARE_TECLADO_H
#define HARDWARE_TECLADO_H

// 1. Inclui a nova interface
#include "../interface/IDispositivoIRQ.h"

#include <string>   // Para std::string
#include <queue>    // Para std::queue
#include <cstdint>  // Para uint8_t
#include <iostream> // Para std::cout, std::endl, std::hex
#include <sstream>  // Para std::stringstream
#include <iomanip>  // Para std::setw, std::setfill

// Constantes públicas
static const uint8_t STATUS_VAZIO = 0x00;
static const uint8_t STATUS_DADOS_PRONTOS = 0x01;

/**
 * @class HardwareTeclado
 * @brief Simula o hardware físico de um teclado (Controlador).
 */
class HardwareTeclado : public IDispositivoIRQ
{
public:
    // --- 1. EVENTOS DE GATILHO EXTERNO ---
    HardwareTeclado();
    void eventoUsuarioDigitou(const std::string &texto);
    void eventoCPULeuDados();

    // --- 2. INTERFACE PÚBLICA (Lida por outras classes) ---
    uint8_t lerStatus() const;
    uint8_t lerDados() const;
    bool estaSinalIRQAtivo() const;

private:
    // --- 3. ESTADO INTERNO DO HARDWARE ---
    std::queue<char> m_bufferInterno;
    uint8_t m_registroStatus;
    uint8_t m_registroDados;
    bool m_sinalIRQAtivo;

    // --- 4. FUNÇÕES DE LÓGICA INTERNA ---
    void _tentarMoverBufferParaRegistrador();
    void _atualizarSinalIRQ();

    // --- 5. HELPER DE LOG ---
    void _log(const std::string &mensagem);
};

#endif // HARDWARE_TECLADO_H