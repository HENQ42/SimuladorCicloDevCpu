#ifndef BUFFER_ENTRADA_OS_H
#define BUFFER_ENTRADA_OS_H

#include <queue>
#include <mutex>

class BufferDeEntradaOS
{
public:
    // Chamado pelo ISR (Produtor)
    void enfileirarTecla(char c)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(c);
    }

    // Chamado pela Aplicação (Consumidor)
    char desenfileirarTecla()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return 0; // 0 = Nenhuma tecla
        char c = m_queue.front();
        m_queue.pop();
        return c;
    }

    // Chamado pela Aplicação (Consumidor)
    bool temDados()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_queue.empty();
    }

private:
    std::mutex m_mutex;
    std::queue<char> m_queue;
};

#endif