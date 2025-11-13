#include "AppDonut.h"

AppDonut::AppDonut() : m_angleA(0), m_angleB(0)
{
    // Estado inicial dos ângulos
}

void AppDonut::conectar(BufferDeEntradaOS *bufferEntrada, IFrameBuffer *framebuffer)
{
    m_bufferEntrada = bufferEntrada;
    m_framebuffer = framebuffer;
}

void AppDonut::executarTick()
{
    if (!m_bufferEntrada || !m_framebuffer)
        return; // Não executa se não estiver conectado

    // --- 1. Processar Inputs (Consumidor) ---
    // (Consome TODAS as teclas no buffer)
    while (m_bufferEntrada->temDados())
    {
        char c = m_bufferEntrada->desenfileirarTecla();
        switch (c)
        {
        case 'w':
            m_angleA -= 0.1;
            break; // Rotaciona "para cima"
        case 's':
            m_angleA += 0.1;
            break; // Rotaciona "para baixo"
        case 'a':
            m_angleB -= 0.1;
            break; // Rotaciona "para esquerda"
        case 'd':
            m_angleB += 0.1;
            break; // Rotaciona "para direita"
        }
    }

    // --- 2. Atualizar Estado (Spin Automático) ---
    // (Isso faz ele girar sozinho, mesmo sem input)
    m_angleA += 0.02;
    m_angleB += 0.01;

    // --- 3. Renderizar ---
    std::string frame_output;
    frame_output.reserve(W * H + 5); // Otimização: aloca memória

    _renderizarFrame(frame_output);

    // --- 4. Enviar para a "Tela" ---
    m_framebuffer->atualizar(frame_output);
}

/**
 * @brief Esta é a sua função, adaptada para C++ e para usar
 * os membros da classe (m_angleA, m_angleB).
 */
void AppDonut::_renderizarFrame(std::string &output)
{
    static const char gradient[] = ".,-~:;=!*#$@";

    // Usamos std::vector em vez de arrays C (mais seguro)
    std::vector<char> b(H * W, ' ');
    std::vector<double> z(H * W, 0.0);

    double A = m_angleA; // Usa o estado da classe
    double B = m_angleB; // Usa o estado da classe

    for (double j = 0; j < 6.28; j += 0.07)
    {
        for (double i = 0; i < 6.28; i += 0.02)
        {
            double c = sin(i), d = cos(j), e = sin(A), f = sin(j), g = cos(A);
            double h = d + 2, D = 1 / (c * h * e + f * g + 5);
            double l = cos(i), m = cos(B), n = sin(B);
            double t = c * h * g - f * e;
            int x = W / 2 + 30 * D * (l * h * m - t * n);
            int y = H / 2 + 15 * D * (l * h * n + t * m);
            int o = x + W * y;
            int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);
            if (H > y && y > 0 && x > 0 && W > x && D > z[o])
            {
                z[o] = D;
                b[o] = gradient[N > 0 ? N : 0];
            }
        }
    }

    // O \x1b[H (home) é crucial para limpar a tela do terminal
    output.clear();
    output += "\x1b[H";

    // Constrói a string de saída (mais eficiente que sprintf/strlen)
    for (int k = 0; k < H * W; k++)
    {
        output += (k % W) ? b[k] : '\n';
    }
}