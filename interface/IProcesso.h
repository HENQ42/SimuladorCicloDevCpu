#ifndef I_APLICACAO_H
#define I_APLICACAO_H

#include "../buffer/BufferDeEntradaOS.h"
#include "../interface/IFrameBuffer.h"

class IAplicacao
{
public:
    virtual ~IAplicacao() = default;

    /**
     * @brief Conecta a aplicação às interfaces do "SO".
     */
    virtual void conectar(BufferDeEntradaOS *bufferEntrada, IFrameBuffer *framebuffer) = 0;

    /**
     * @brief Executa um tick de lógica da aplicação.
     * (Chamado pela CPU quando não há interrupções).
     */
    virtual void executarTick() = 0;
};

#endif