#ifndef I_FRAMEBUFFER_H
#define I_FRAMEBUFFER_H

#include <string>

class IFrameBuffer
{
public:
    virtual ~IFrameBuffer() = default;

    /**
     * @brief Atualiza todo o conteúdo do framebuffer.
     * (Simulando um display de terminal/texto).
     */
    virtual void atualizar(const std::string &conteudo) = 0;

    /**
     * @brief Limpa o framebuffer.
     */
    virtual void limpar() = 0;
};

#endif