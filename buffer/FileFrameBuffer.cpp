#include "FileFrameBuffer.h"

FileFrameBuffer::FileFrameBuffer(const std::string& caminhoArquivo)
    : m_caminhoArquivo(caminhoArquivo) {
    // Limpa o arquivo no início
    std::ofstream out(m_caminhoArquivo, std::ios::trunc);
    out.close();
}

// O código do donut já envia \x1b[H (home), 
// que limpa a tela do terminal, então não precisamos
// implementar uma limpeza de arquivo separada.
void FileFrameBuffer::limpar() {
    // Não é necessário para este caso
}

void FileFrameBuffer::atualizar(const std::string& conteudo) {
    // Abre o arquivo (trunc = apaga o anterior) e escreve o frame
    std::ofstream out(m_caminhoArquivo, std::ios::trunc);
    if (out.is_open()) {
        out << conteudo;
        out.close();
    }
}

