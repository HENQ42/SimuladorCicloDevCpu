#!/bin/bash

# 1. Apaga o conteúdo do arquivo de log (cria o arquivo se não existir)
echo "Limpando sim_logs.txt..."
> sim_logs.txt

# 2. Define o nome da sessão tmux
SESSION_NAME="sim"

# 3. Mata qualquer sessão tmux com o mesmo nome (para recomeçar limpo)
tmux kill-session -t $SESSION_NAME 2>/dev/null

# 4. Inicia uma nova sessão tmux desanexada
echo "Iniciando sessão tmux: $SESSION_NAME"
tmux new-session -d -s $SESSION_NAME

# 5. Configura o layout
# Divide a janela principal em duas colunas (esquerda 60%, direita 40%)
tmux split-window -h -p 40
# Pane 0 = esquerda (60%)
# Pane 1 = direita (40%)

# Seleciona o painel da esquerda (0)
tmux select-pane -t 0

# Divide o painel da esquerda horizontalmente (50/50)
tmux split-window -v -p 50
# Pane 0 = top-left
# Pane 2 = bottom-left
# Pane 1 = side-right

# 6. Envia os comandos para cada painel
echo "Iniciando processos..."

# Pane 0 (top-left): O 'watch' do framebuffer
tmux send-keys -t 0 "watch -n 0.1 cat sim_frame.txt" C-m

# Pane 1 (side-right): O listener de input
tmux send-keys -t 1 "./listener" C-m 

# Pane 2 (bottom-left): O 'tail' dos logs
tmux send-keys -t 2 "tail -f sim_logs.txt" C-m

# 7. Foca no painel do listener e anexa à sessão
tmux select-pane -t 1 # <-- CORREÇÃO AQUI (era -t 2)
tmux attach-session -t $SESSION_NAME

echo "Sessão encerrada."