#ifndef APP_DONUT_H
#define APP_DONUT_H

#include <string>
#include <vector>
#include <cmath>   // Para sin() e cos()
#include <cstring> // Para memset()
#include "../interface/IProcesso.h"
#include "../buffer/BufferDeEntradaOS.h"
#include "../interface/IFrameBuffer.h"

// Definindo o tamanho do nosso "framebuffer"
static const int W = 80;
static const int H = 24;

class AppDonut : public IAplicacao
{
public:
    AppDonut();
    virtual ~AppDonut() = default;

    /**
     * @brief Conecta a aplicação às interfaces do "SO".
     * (Implementação do contrato IAplicacao)
     */
    void conectar(BufferDeEntradaOS *bufferEntrada, IFrameBuffer *framebuffer) override;

    /**
     * @brief Executa um tick de lógica da aplicação (o "game loop").
     * (Implementação do contrato IAplicacao)
     */
    void executarTick() override;

private:
    // --- Interfaces do "SO" ---
    BufferDeEntradaOS *m_bufferEntrada = nullptr;
    IFrameBuffer *m_framebuffer = nullptr;

    // --- Estado Interno da Aplicação ---
    double m_angleA;
    double m_angleB;
    // (O ângulo C do seu código não era usado, então vamos focar em A e B)

    /**
     * @brief Renderiza um único frame do donut na string de output.
     * (Este é o seu código, adaptado para C++ e OOP).
     */
    void _renderizarFrame(std::string &output);
};

#endif // APP_DONUT_H