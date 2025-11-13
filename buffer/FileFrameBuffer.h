#ifndef FILE_FRAMEBUFFER_H
#define FILE_FRAMEBUFFER_H

#include "../interface/IFrameBuffer.h"
#include <string>
#include <fstream> // Para std::ofstream

/**
 * @class FileFrameBuffer
 * @brief Implementação concreta do IFrameBuffer que escreve
 * o frame em um arquivo de texto.
 */
class FileFrameBuffer : public IFrameBuffer {
public:
    FileFrameBuffer(const std::string& caminhoArquivo);
    virtual ~FileFrameBuffer() = default;

    // Removemos o 'limpar()' pois \x1b[H faz isso
    void limpar() override;
    void atualizar(const std::string& conteudo) override;

private:
    std::string m_caminhoArquivo;
};

#endif // FILE_FRAMEBUFFER_H

