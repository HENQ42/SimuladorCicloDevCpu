#include <iostream>
#include <string>
#include <iomanip>    // Para std::hex, std::dec
#include <functional> // Para std::function

#include <thread> // Para std::this_thread
#include <chrono> // Para std::chrono

#include "./teclado/teclado.h"
#include "./pic/ControladorPIC.h" // (Assumindo que define ControladorPIC)
#include "./cpu/cpu.h"

#include "./interface/IProcesso.h"
#include "./interface/IFrameBuffer.h"

#include "./app/donut.h"

// ---
// --- BLOCO 1: OBJETOS MOCK (FALSOS) PARA O TESTE
// ---

/**
 * @class MockFrameBuffer
 * @brief Um "framebuffer" falso que implementa a interface
 * IFrameBuffer, mas apenas imprime no console (std::cout).
 */
class MockFrameBuffer : public IFrameBuffer
{
public:
    void limpar() override
    {
// Simula a limpeza de tela no console
#ifdef _WIN32
        std::system("cls");
#else
        std::system("clear");
#endif
    }

    void atualizar(const std::string &conteudo) override
    {
        // Imprime o "frame"
        std::cout << "--- [FRAMEBUFFER] ---" << std::endl;
        std::cout << conteudo << std::endl;
        std::cout << "---------------------" << std::endl;
    }
};

/**
 * @class AppProcessadorDeEco
 * @brief Uma "aplicação" simples que implementa IAplicacao.
 * Ela lê do buffer de entrada e "ecoA" no framebuffer.
 */
class AppProcessadorDeEco : public IAplicacao
{
private:
    BufferDeEntradaOS *m_bufferEntrada = nullptr;
    IFrameBuffer *m_framebuffer = nullptr;
    std::string m_textoAtual = "Digite algo (simulando socket): ";

public:
    virtual ~AppProcessadorDeEco() = default;

    void conectar(BufferDeEntradaOS *bufferEntrada, IFrameBuffer *framebuffer) override
    {
        m_bufferEntrada = bufferEntrada;
        m_framebuffer = framebuffer;
    }

    void executarTick() override
    {
        if (!m_bufferEntrada || !m_framebuffer)
            return;

        // 1. Processar Input (se houver)
        if (m_bufferEntrada->temDados())
        {
            char c = m_bufferEntrada->desenfileirarTecla();
            m_textoAtual += c; // Adiciona a tecla ao nosso "frame"
        }

        // 2. Renderizar
        // (Em um app real, faríamos isso em um timer, mas aqui vamos
        // renderizar a cada tick para ver o resultado)
        m_framebuffer->limpar();
        m_framebuffer->atualizar(m_textoAtual);
    }
};

// ---
// --- BLOCO 2: O TESTE DE INTEGRAÇÃO
// ---

int main()
{
    std::cout << "Iniciando Teste de Integração Completo (com Mocks)..." << std::endl;
    std::cout << "==================================================" << std::endl;

    // --- 1. Criar os "Serviços" e "Mocks" ---
    BufferDeEntradaOS bufferDeEntrada;
    MockFrameBuffer telaMock; // Nosso "mmap" falso

    // --- 2. Criar o Hardware ---
    HardwareTeclado teclado; // (Já é thread-safe)
    ControladorPIC pic;
    CPU cpu(pic); // CPU depende da interface IControladorIRQ

    // --- 3. Criar a Aplicação Concreta ---
    AppProcessadorDeEco appEco;

    // --- 4. Fazer a "Fiação" (Injeção de Dependência) ---

    // 4a. Conectar Hardware
    pic.registrarDispositivo(1, &teclado); // PIC <- Teclado (IIRQDevice)

    // 4b. Instalar o Driver (ISR)
    cpu.registrarISR(1, [&teclado, &bufferDeEntrada]()
                     {
        // ISR lê do hardware...
        char c = (char)teclado.lerDados();
        // ...e coloca no buffer do SO.
        bufferDeEntrada.enfileirarTecla(c);
        teclado.eventoCPULeuDados(); });

    // 4c. Conectar a Aplicação
    appEco.conectar(&bufferDeEntrada, &telaMock);

    // 4d. Carregar a Aplicação na CPU
    cpu.carregarAplicacao(&appEco);

    std::cout << "Sistema montado. Iniciando simulação..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- 5. Iniciar Simulação ---
    // Em um cenário real, o `cpu.tick()` e o `teclado.evento...`
    // rodariam em threads separadas.
    // Aqui, vamos simular isso em um loop único para o teste.

    std::cout << "Simulando 'A'..." << std::endl;
    teclado.eventoUsuarioDigitou("A"); // (Socket na Thread B)
    cpu.tick();                        // (CPU na Thread A)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // O 1º tick da App (sem IRQ) vai consumir o 'A' do buffer
    cpu.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Simulando 'B'..." << std::endl;
    teclado.eventoUsuarioDigitou("B"); // (Socket)
    cpu.tick();                        // (CPU detecta IRQ de 'B')
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cpu.tick(); // (App consome 'B')
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\n==================================================" << std::endl;
    std::cout << "Teste de integração concluído." << std::endl;

    return 0;
}
