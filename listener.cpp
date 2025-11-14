#include <iostream>
#include <fstream>
#include <unistd.h>    // Para read(), STDIN_FILENO, tcsetattr, tcgetattr
#include <termios.h>   // Para a mágica do terminal (modo raw)

/**
 * @struct RawMode
 * @brief Uma classe simples que usa o padrão RAII.
 * Ela ativa o modo "raw" do terminal no construtor
 * e garante que o modo antigo seja restaurado no destrutor.
 * Isso é crucial para não quebrar seu terminal se o programa travar.
 */
struct RawMode {
    termios old_settings;

    RawMode() {
        // 1. Salva as configurações atuais do terminal
        tcgetattr(STDIN_FILENO, &old_settings);
        
        termios new_settings = old_settings;
        
        // 2. Desativa o modo canônico (ICANON) e o eco (ECHO)
        // ICANON = Não esperar por "Enter"
        // ECHO = Não imprimir a tecla digitada no console
        new_settings.c_lflag &= ~(ICANON | ECHO);
        
        // 3. Aplica as novas configurações imediatamente
        tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    }

    ~RawMode() {
        // 4. Restaura as configurações originais quando o objeto sai de escopo
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    }
};

int main() {
    // 1. Ativa o modo raw.
    // O destrutor será chamado automaticamente no fim do 'main'.
    RawMode raw; 

    std::cout << "Ouvindo teclas... Pressione '.' (ponto) para sair." << std::endl;

    char c = 0;
    // 2. Loop principal: lê 1 byte (um char) do STDIN
    while (read(STDIN_FILENO, &c, 1) == 1) {
        
        // 3. Condição de saída
        if (c == '.') {
            std::cout << "Saindo..." << std::endl;
            break;
        }

        // 4. Abre o arquivo, apagando o conteúdo anterior (trunc)
        std::ofstream out("sim_input.txt", std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "Erro: Não foi possível abrir sim_input.txt" << std::endl;
            continue; 
        }
        
        out << c; // 5. Escreve o único caractere
        out.close();

        // Feedback visual no console (opcional)
        std::cout << "-> '" << c << "' enviado." << std::endl;
    }

    return 0;
}