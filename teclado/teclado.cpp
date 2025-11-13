#include "teclado.h"

// Includes necessários para os logs corretos
#include <iostream>
#include <string>
#include <sstream> // Para std::stringstream
#include <iomanip> // Para std::hex, std::setw, std::setfill
#include <cstdint> // Para uint8_t

// --- CONSTRUTOR ---
HardwareTeclado::HardwareTeclado()
    : m_registroStatus(STATUS_VAZIO),
      m_registroDados(0x00),
      m_sinalIRQAtivo(false)
{
    _log("Hardware inicializado. Estado: Ocioso.");
}

// --- 1. EVENTOS DE GATILHO EXTERNO ---

void HardwareTeclado::eventoUsuarioDigitou(const std::string &texto)
{
    if (texto.empty())
    {
        return;
    }
    _log("Recebendo digitação do usuário (" + std::to_string(texto.length()) + " tecla(s)).");

    for (char c : texto)
    {
        m_bufferInterno.push(c);

        // --- LOG CORRIGIDO (usa a mesma lógica do main.cpp) ---
        std::string msg = "Tecla '";
        msg += c;
        msg += "' (";
        std::stringstream ss; // Usa stringstream para formatar
        ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)static_cast<uint8_t>(c);
        msg += ss.str();
        msg += ") enfileirada no buffer.";
        _log(msg);
    }
    _tentarMoverBufferParaRegistrador();
}

void HardwareTeclado::eventoCPULeuDados()
{
    if (m_registroStatus == STATUS_VAZIO)
    {
        _log("AVISO: CPU leu registradores, mas o status já era VAZIO.");
        return;
    }

    // --- LOG CORRIGIDO (usa stringstream) ---
    std::stringstream ss;
    ss << "CPU/ISR leu o dado (0x" << std::hex << (int)m_registroDados << "). Limpando status.";
    _log(ss.str());

    // --- CORREÇÃO DO BUG 2: Limpa os registradores ---
    m_registroStatus = STATUS_VAZIO; // Marca como lido
    m_registroDados = 0x00;          // Zera o registrador de dados (limpa o dado "sujo")
    // ----------------------------------------------

    _atualizarSinalIRQ();                // Sinal IRQ será desativado (pois status é VAZIO)
    _tentarMoverBufferParaRegistrador(); // Tenta puxar o próximo item do buffer (ex: 'L' de "OLA")
}

// --- 2. INTERFACE PÚBLICA (Getters/Leitores MMIO) ---

uint8_t HardwareTeclado::lerStatus() const
{
    return m_registroStatus;
}

uint8_t HardwareTeclado::lerDados() const
{
    return m_registroDados;
}

bool HardwareTeclado::estaSinalIRQAtivo() const
{
    return m_sinalIRQAtivo;
}

// --- 4. FUNÇÕES DE LÓGICA INTERNA (Privadas) ---

void HardwareTeclado::_tentarMoverBufferParaRegistrador()
{
    if (m_registroStatus == STATUS_VAZIO && !m_bufferInterno.empty())
    {
        char scancode = m_bufferInterno.front();
        m_bufferInterno.pop();

        m_registroDados = static_cast<uint8_t>(scancode);
        m_registroStatus = STATUS_DADOS_PRONTOS;

        // --- LOG CORRIGIDO (usa stringstream) ---
        std::stringstream ss;
        ss << "Movendo dado do buffer (0x" << std::hex << (int)m_registroDados << ") para registradores MMIO.";
        _log(ss.str());

        _atualizarSinalIRQ(); // Sinal IRQ será ativado
    }
}

void HardwareTeclado::_atualizarSinalIRQ()
{
    bool novoEstadoIRQ = (m_registroStatus == STATUS_DADOS_PRONTOS);
    if (novoEstadoIRQ != m_sinalIRQAtivo)
    {
        m_sinalIRQAtivo = novoEstadoIRQ;
        if (m_sinalIRQAtivo)
        {
            _log("Sinal IRQ definido para ATIVO.");
        }
        else
        {
            _log("Sinal IRQ definido para INATIVO.");
        }
    }
}

// --- 5. HELPER DE LOG ---

void HardwareTeclado::_log(const std::string &mensagem)
{
    std::cout << "[TECLADO HARDWARE] " << mensagem << std::endl;
}