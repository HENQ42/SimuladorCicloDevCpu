#include <iostream>
#include <fstream>
#include <sstream> // <--- NECESSÁRIO PARA LER O ARQUIVO
#include <string>
#include <thread>
#include <chrono>

// --- mmap() NÃO SÃO MAIS NECESSÁRIOS ---
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
// ------------------------------------

#include <ncurses.h> // Para a interface do terminal

// --- Nossos arquivos de interface ---
const char* ARQUIVO_LOGS = "sim_logs.txt";
const char* ARQUIVO_FRAME = "sim_frame.txt";
const char* ARQUIVO_INPUT = "sim_input.txt";

// --- Constantes de dimensão ---
static const int W = 80;
static const int H = 24;

/**
 * @brief Escreve uma string no arquivo de input.
 */
void enviarInput(const std::string& input) {
    std::ofstream out(ARQUIVO_INPUT, std::ios::trunc);
    out << input;
    out.close();
}

/**
 * @brief Adiciona uma entrada de log.
 */
void logTestador(const std::string& mensagem) {
    std::ofstream log(ARQUIVO_LOGS, std::ios::app);
    log << "[TESTADOR] " << mensagem << std::endl;
    log.close();
}

/**
 * @brief Lê o framebuffer com std::ifstream (seguro)
 * e o desenha na janela.
 */
void desenharFrame(WINDOW* win) {
    std::ifstream frameFile(ARQUIVO_FRAME);
    if (!frameFile.is_open()) {
        mvwprintw(win, 1, 1, "Erro: Nao foi possivel abrir 'sim_frame.txt'");
        wrefresh(win);
        return;
    }

    // Lê o arquivo inteiro para um buffer de string
    std::stringstream buffer;
    buffer << frameFile.rdbuf();
    std::string frameConteudo = buffer.str();
    frameFile.close();

    // Desenha o conteúdo lido na janela
    wclear(win);
    box(win, 0, 0); // Redesenha a borda
    mvwprintw(win, 0, 2, "[ Output (Leitura de Arquivo) ]");
    
    // Imprime o buffer lido (usa .c_str() para ncurses)
    mvwprintw(win, 1, 1, frameConteudo.c_str()); 
    wrefresh(win);

    // Não há mmap, não há nada para "desfazer"
}

int main() {
    // --- Configuração do ncurses ---
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Cria a janela de output (o donut)
    int frameHeight = H + 2; 
    WINDOW* frameWin = newwin(frameHeight, W + 2, 0, 0);
    box(frameWin, 0, 0);
    mvwprintw(frameWin, 0, 2, "[ Output (Leitura de Arquivo) ]");
    wrefresh(frameWin);

    // Cria a janela de input
    WINDOW* inputWin = newwin(5, width, frameHeight, 0);
    box(inputWin, 0, 0);
    mvwprintw(inputWin, 0, 2, "[ Input ]");
    mvwprintw(inputWin, 1, 1, "Pressione 'w', 'a', 's', 'd'. Pressione 'q' para sair.");
    wrefresh(inputWin);
    // ----------------------------

    logTestador("Testador iniciado.");

    // --- Loop Principal do Testador ---
    while (true) {
        // 1. Desenha o frame lido do arquivo
        desenharFrame(frameWin);

        // 2. Verifica se o usuário digitou algo
        int ch = getch(); // Pega a tecla (não bloqueante)

        if (ch != ERR) {
            std::string inputStr(1, (char)ch);

            if (ch == 'q') {
                logTestador("Testador finalizado.");
                break; // Sai do loop
            }

            if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
                enviarInput(inputStr); // Escreve em 'sim_input.txt'
                logTestador("Enviei input: " + inputStr);
                
                mvwprintw(inputWin, 2, 1, "ENVIADO: '%c'  ", ch);
                wrefresh(inputWin);
            }
        }

        // Atualiza a 30 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // --- Limpeza do ncurses ---
    endwin();
    return 0;
}

