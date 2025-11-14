#dependences
sudo pacman -S websocketpp asio openssl ncurses boost

#compilar simulador
g++ simulador.cpp ./teclado/teclado.cpp ./pic/ControladorPIC.cpp ./cpu/cpu.cpp ./buffer/FileFrameBuffer.cpp ./app/donut.cpp -o simulador -std=c++17 -pthread

#compilar listener
g++ -o listener listener.cpp -Wall


