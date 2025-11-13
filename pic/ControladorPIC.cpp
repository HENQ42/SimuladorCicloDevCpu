#include "ControladorPIC.h"
#include <string> // Para std::to_string

ControladorPIC::ControladorPIC()
{
    _log("Controlador PIC inicializado.");
}

void ControladorPIC::registrarDispositivo(int linha, IDispositivoIRQ *dispositivo)
{
    if (dispositivo != nullptr)
    {
        m_canaisIRQ[linha] = dispositivo;
        _log("Dispositivo registrado no canal IRQ " + std::to_string(linha) + ".");
    }
}

int ControladorPIC::verificarInterrupcoes()
{
    // Itera por todos os canais registrados
    // (Um PIC real prioriza, mas vamos apenas pegar o primeiro que virmos)
    for (const auto &par : m_canaisIRQ)
    {
        int linha = par.first;
        IDispositivoIRQ *dispositivo = par.second;

        // Pergunta ao dispositivo (sem saber o que ele é): "Sinal ativo?"
        if (dispositivo != nullptr && dispositivo->estaSinalIRQAtivo())
        {
            _log("[PIC] IRQ " + std::to_string(linha) + " ATIVA! Sinalizando CPU...");
            return linha; // Retorna a linha que precisa de atenção
        }
    }

    // Nenhum canal ativo
    return -1;
}

void ControladorPIC::_log(const std::string &mensagem)
{
    std::cout << "[PIC] " << mensagem << std::endl;
}