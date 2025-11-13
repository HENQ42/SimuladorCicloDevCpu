#compilar simulador
g++ simulador.cpp ./teclado/teclado.cpp ./pic/ControladorPIC.cpp ./cpu/cpu.cpp ./buffer/FileFrameBuffer.cpp ./app/donut.cpp -o simulador -std=c++17 -pthread

#compilar teste
g++ testador.cpp -o testador -std=c++17 -pthread -lncurses

#dependences
sudo pacman -S websocketpp asio openssl ncurses boost

#compilar web socket server
g++ websocket.cpp ./teclado/teclado.cpp ./pic/ControladorPIC.cpp ./cpu/cpu.cpp ./app/donut.cpp -o socketweb -std=c++17 -pthread -lssl -lcrypto

