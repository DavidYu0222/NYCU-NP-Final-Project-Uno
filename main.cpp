#include <SFML/Graphics.hpp>
#include <string>
#include <string.h>
#include <iostream>
#include<cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <sys/stat.h>
#define READ_PATH "gui_read.fifo"
#define WRITE_PATH "gui_write.fifo"
using namespace std;
ssize_t read_n(int fd, char *buffer, size_t max_len) {
    size_t bytes_read = 0;
    char ch;
    while (bytes_read < max_len) { 
        ssize_t result = read(fd, &ch, 1);
        if (result <= 0) {
            break;
        }
        if (ch == '\0') {
            break;  // Stop reading at null terminator
        }
        buffer[bytes_read++] = ch;
    }
    buffer[max_len - 1] = '\0';  // Null-terminate the string
    return bytes_read;
}

void Write(int fd, void *ptr, size_t nbytes)
{
	// if (write(fd, ptr, nbytes) != nbytes)
	// 	return;
    write(fd, ptr, nbytes);
}



sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
sf::RenderWindow window(desktopMode, "SFML Windowed Fullscreen", sf::Style::Titlebar | sf::Style::Close);
// sf::RenderWindow window(desktopMode, "Full-Screen Window", sf::Style::Fullscreen);
pid_t pid;

bool isMouseOverButton(const sf::RectangleShape& button, const sf::Vector2i& mousePos) {
    return button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
}

void sig_child(int signo){
	pid_t pid;
	int stat;
	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

class Game{
    public:
        Game(){
            username = "";
            server_ip = "";
            gamestatus = 0;
            font.loadFromFile("../assets/Arial.ttf");
            basic_setting = 1;
        };
        void setusername(string input);
        void drawbackground();
        void setbackground(string path);
        void setserver_ip(char* ip);
        void setgamestatus(int state);
        int getgamestatus();
        void start();
        string Read();
        void sign_in_up();
        void sign_up();
        bool basic_setting;
        void sign_in();
        void create_or_join();
        void join_room();
        string username;
        string room_id;
        string fail_mes; 
        sf::Sprite background;
        int gui_read;
        int gui_write;
    private:
        string server_ip;
        int gamestatus;
        sf::Font font;
        sf::Sprite stack_card;
        vector<sf::Sprite> cards;
};

int main(int argc, char** argv)
{
    signal(SIGCHLD, sig_child);
    window.setPosition(sf::Vector2i(0, 0)); //for 雙螢幕

    Game game;

    mkfifo(WRITE_PATH, 0666);
    mkfifo(READ_PATH, 0666);
    game.gui_write = open(WRITE_PATH, O_WRONLY | O_NONBLOCK);
    game.gui_read = open(READ_PATH, O_RDONLY | O_NONBLOCK);

    game.setserver_ip(argv[1]);    //設定 server ip
    string path = "../assets/room.jpg";
    // game.setbackground(path); //first background
    sf::Texture texture;
    if(!texture.loadFromFile(path)){
        cout << "background error" << endl;
    }
    game.background.setTexture(texture);
    sf::Vector2u textureSize = texture.getSize(); // 圖片大小
    sf::Vector2u windowSize = window.getSize();             // 視窗大小
    float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
    float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
    game.background.setScale(scaleX, scaleY);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape){
                window.close(); // 按下 ESC 關閉視窗
            }
        }
        
        window.clear();
        // game.drawbackground();
        if(game.getgamestatus() == 0){    //設定用戶名稱頁面
            game.start();
        }
        
        else if(game.getgamestatus() == 1){     //登入 or 註冊
            game.sign_in_up();
        }
        else if(game.getgamestatus() == 2){     //sign in
            game.sign_in();
        }
        else if(game.getgamestatus() == 3){     //sign up
            game.sign_up();
        }
        else if(game.getgamestatus() == 4){     //選擇創建房間or進入房間
            game.create_or_join();
        }
        else if(game.getgamestatus() == 5){    //加入房間
            game.join_room();
        }
        else if(game.getgamestatus() == 6){    
            
        }
        else if(game.getgamestatus() == 4){     //已進入房間
            if(game.basic_setting){
                string path = "../assets/background.jpg";
                sf::Texture texture;
                sf::Sprite background;
                if(!texture.loadFromFile(path)){
                    cout << "background error" << endl;
                }
                background.setTexture(texture);
                sf::Vector2u textureSize = texture.getSize(); // 圖片大小
                sf::Vector2u windowSize = window.getSize();             // 視窗大小
                float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
                float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
                background.setScale(scaleX, scaleY);
            }
        }
        else if(game.getgamestatus() ==6){      //登入
            if(game.basic_setting){
            }

        }
        else if(game.getgamestatus() == 7){     //註冊

        }
        window.display();

    }
    kill(pid, SIGINT);
    close(game.gui_read);
    close(game.gui_write);
    unlink(WRITE_PATH);
    unlink(READ_PATH);
    return EXIT_SUCCESS;
}

void Game::setusername(string input){
    username = input;
}

void Game::setbackground(string path){
    sf::Texture texture;
    if(!texture.loadFromFile(path)){
        cout << "background error" << endl;
    }
    background.setTexture(texture);
    cout << "set background" << endl;
}

void Game::drawbackground(){
    window.draw(background);
}

void Game::setserver_ip(char* ip){
    server_ip = ip;
}

void Game::setgamestatus(int state){
    gamestatus = state;
}

int Game::getgamestatus(){
    return gamestatus;
}

string Game::Read(){
    char buffer[4096];
    fd_set rset;
    FD_SET(gui_read, &rset);
    int maxfd = gui_read + 1;
    select(maxfd, &rset, NULL,NULL,NULL);
    read_n(gui_read, buffer, sizeof(buffer));
    string str(buffer);
    return str;
}

void Game::start(){
    sf::RectangleShape inputBox;
    sf::Text text;
    sf::Text hint_message;
    //input box setting
    inputBox.setSize(sf::Vector2f(300, 50));
    inputBox.setPosition(90, 90);
    inputBox.setFillColor(sf::Color::White);
    inputBox.setOutlineThickness(2);
    inputBox.setOutlineColor(sf::Color::Black);
    //text setting
    text.setFont(font);  // 設定字型
    text.setCharacterSize(20);  // 設定字體大小
    text.setString("");
    text.setPosition(60, 85); // 確保文字顯示在矩形框內
    text.setFillColor(sf::Color::Black);
    //hint setting
    hint_message.setFont(font);
    hint_message.setCharacterSize(20);  // 設定字體大小
    hint_message.setString("Please enter your username: ");
    hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
    hint_message.setFillColor(sf::Color::Red);
    string tmp = "";
    sf::Event event;
    while(window.isOpen()){
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    setgamestatus(1);
                    string command = "../unpv13e/NYCU-NP-Final-Project-Uno/cli " + server_ip + " " + tmp;
                    username = tmp;
                    cout << command << endl;
                    if((pid = fork()) == 0){
                        system(command.c_str());    //can open, but cli.c have problem
                        exit(0);
                    }
                    string buf = Read();
                    cout << "read from fifo" << buf << endl;
                    return;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
    }
}

void Game::sign_in_up(){
    sf::RectangleShape signin;
    sf::RectangleShape signup;
    sf::Text signinText;
    sf::Text signupText;
    signin.setSize(sf::Vector2f(200, 50));
    signin.setPosition(100, 200);
    signin.setFillColor(sf::Color::Blue);
    signinText.setFont(font);
    signinText.setCharacterSize(20);  // 設定字體大小
    signinText.setString("Sign in");
    signinText.setFillColor(sf::Color::White);
    signinText.setPosition(
        signin.getPosition().x + (signin.getSize().x - signinText.getLocalBounds().width) / 2,
        signin.getPosition().y + (signin.getSize().y - signinText.getLocalBounds().height) / 2 - 5
    );
    signup.setSize(sf::Vector2f(200, 50));
    signup.setPosition(100, 300);
    signup.setFillColor(sf::Color::Green);
    signupText.setFont(font);
    signupText.setCharacterSize(20);  // 設定字體大小
    signupText.setString("Sign up");
    signupText.setFillColor(sf::Color::White);
    signupText.setPosition(
        signup.getPosition().x + (signup.getSize().x - signupText.getLocalBounds().width) / 2,
        signup.getPosition().y + (signup.getSize().y - signupText.getLocalBounds().height) / 2 - 5
    );
    sf::Text hint_message;
    if(fail_mes != ""){
        hint_message.setFont(font);
        hint_message.setCharacterSize(20);  // 設定字體大小
        hint_message.setString(fail_mes);
        hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
        hint_message.setFillColor(sf::Color::Red);
    }
    while(window.isOpen()){
        window.clear();
        sf::Event event;
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();

            // 滑鼠點擊檢測
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (isMouseOverButton(signin, mousePos)) {     //sign in bug
                    fail_mes = "";
                    char* command = (char *)"0\n";
                    Write(gui_write, command, sizeof(command));
                    string buf = Read();
                    cout << "read from fifo" << buf << endl;
                    setgamestatus(2);
                    return;
                } else if (isMouseOverButton(signup, mousePos)) {  //sign up
                    char* command = "1\n";
                    Write(gui_write, command, sizeof(command));
                    string buf = Read();
                    cout << "read from fifo" << buf << endl;
                    fail_mes = "";
                    setgamestatus(3);
                    return;
                }
            }
        }
        window.draw(background);
        window.draw(signin);
        window.draw(signup);
        window.draw(signinText);
        window.draw(signupText);
        window.draw(hint_message);
        window.display();
    }
}

void Game::sign_in(){
    sf::RectangleShape inputBox;
    sf::Text text;
    sf::Text hint_message;
    //input box setting
    inputBox.setSize(sf::Vector2f(300, 50));
    inputBox.setPosition(90, 90);
    inputBox.setFillColor(sf::Color::White);
    inputBox.setOutlineThickness(2);
    inputBox.setOutlineColor(sf::Color::Black);
    //text setting
    text.setFont(font);  // 設定字型
    text.setCharacterSize(20);  // 設定字體大小
    text.setString("");
    text.setPosition(60, 85); // 確保文字顯示在矩形框內
    text.setFillColor(sf::Color::Black);
    //hint setting
    hint_message.setFont(font);
    hint_message.setCharacterSize(20);  // 設定字體大小
    hint_message.setString("Please enter your username: ");
    hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
    hint_message.setFillColor(sf::Color::Red);
    string tmp = "";
    sf::Event event;
    int stage = 0;
    while(window.isOpen()){     //username
        window.draw(inputBox);
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    char* command;
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    stage = 1;
                    break;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        if(stage == 1){
            stage = 0;
            break;
        }
    }
    tmp = "";
    hint_message.setString("Please enter your password: ");
    while(window.isOpen()){
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    char* command;
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    string buf = Read();         //check whether successful
                    if(buf == "Error: Password Not Match"){
                        fail_mes = buf;
                    }
                    break;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
    }
}

void Game::sign_up(){
    sf::RectangleShape inputBox;
    sf::Text text;
    sf::Text hint_message;
    //input box setting
    inputBox.setSize(sf::Vector2f(300, 50));
    inputBox.setPosition(90, 90);
    inputBox.setFillColor(sf::Color::White);
    inputBox.setOutlineThickness(2);
    inputBox.setOutlineColor(sf::Color::Black);
    //text setting
    text.setFont(font);  // 設定字型
    text.setCharacterSize(20);  // 設定字體大小
    text.setString("");
    text.setPosition(60, 85); // 確保文字顯示在矩形框內
    text.setFillColor(sf::Color::Black);
    //hint setting
    hint_message.setFont(font);
    hint_message.setCharacterSize(20);  // 設定字體大小
    hint_message.setString("Please enter your username: ");
    hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
    hint_message.setFillColor(sf::Color::Red);
    string tmp = "";
    sf::Event event;
    int stage = 0;
    while(window.isOpen()){     //username
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    char* command;
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    stage = 1;
                    break;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
        if(stage == 1){
            stage = 0;
            break;
        }
    }
    tmp = "";
    hint_message.setString("Please enter your password: ");
    while(window.isOpen()){
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    char* command;
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    string buf = Read();         //check whether successful
                    stage = 1;
                    break;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
        if(stage){
            stage = 0;
            break;
        }
    }
    stage = 0;
    string passwd = tmp;
    tmp = "";
    hint_message.setString("Retype your password");
    while(window.isOpen()){
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    char* command;
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    string buf = Read();         //check whether successful
                    if(buf == "Error: Exceed the limitation of string length"){
                        fail_mes = buf;
                    }
                }
                return;
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
    }
}

void Game::create_or_join(){
    sf::RectangleShape create;
    sf::RectangleShape join;
    sf::Text createText;
    sf::Text joinText;
    create.setSize(sf::Vector2f(200, 50));
    create.setPosition(100, 200);
    create.setFillColor(sf::Color::Blue);
    createText.setFont(font);
    createText.setCharacterSize(20);  // 設定字體大小
    createText.setString("Create room");
    createText.setFillColor(sf::Color::White);
    createText.setPosition(
        create.getPosition().x + (create.getSize().x - createText.getLocalBounds().width) / 2,
        create.getPosition().y + (create.getSize().y - createText.getLocalBounds().height) / 2 - 5
    );
    join.setSize(sf::Vector2f(200, 50));
    join.setPosition(100, 300);
    join.setFillColor(sf::Color::Green);
    joinText.setFont(font);
    joinText.setCharacterSize(20);  // 設定字體大小
    joinText.setString("Join room");
    joinText.setFillColor(sf::Color::White);
    joinText.setPosition(
        join.getPosition().x + (join.getSize().x - joinText.getLocalBounds().width) / 2,
        join.getPosition().y + (join.getSize().y - joinText.getLocalBounds().height) / 2 - 5
    );
    sf::Text hint_message;
    if(fail_mes != ""){
        hint_message.setFont(font);
        hint_message.setCharacterSize(20);  // 設定字體大小
        hint_message.setString(fail_mes);
        hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
        hint_message.setFillColor(sf::Color::Red);
    }
    while(window.isOpen()){
        window.clear();
        sf::Event event;
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();

            // 滑鼠點擊檢測
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (isMouseOverButton(create, mousePos)) {     //create room
                    char* command = "create_room\n";
                    Write(gui_write, command, sizeof(command));
                    setgamestatus(2);
                    return;
                } else if (isMouseOverButton(join, mousePos)) {  //join room
                    setgamestatus(3);
                    return;
                }
            }
        }
        window.draw(background);
        window.draw(create);
        window.draw(join);
        window.draw(createText);
        window.draw(joinText);
        window.draw(hint_message);
        window.display();
    }
}

void Game::join_room(){
    sf::RectangleShape inputBox;
    sf::Text text;
    sf::Text hint_message;
    //input box setting
    inputBox.setSize(sf::Vector2f(300, 50));
    inputBox.setPosition(90, 90);
    inputBox.setFillColor(sf::Color::White);
    inputBox.setOutlineThickness(2);
    inputBox.setOutlineColor(sf::Color::Black);
    //text setting
    text.setFont(font);  // 設定字型
    text.setCharacterSize(20);  // 設定字體大小
    text.setString("");
    text.setPosition(60, 85); // 確保文字顯示在矩形框內
    text.setFillColor(sf::Color::Black);
    //hint setting
    hint_message.setFont(font);
    hint_message.setCharacterSize(20);  // 設定字體大小
    hint_message.setString("Enter the room id: ");
    hint_message.setPosition(40, 85); // 確保文字顯示在矩形框內
    hint_message.setFillColor(sf::Color::Red);
    string tmp = "";
    sf::Event event;
    while(window.isOpen()){
        window.clear();
        while(window.pollEvent(event)){
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // 處理退格鍵
                    if (!tmp.empty()) {
                        tmp.pop_back(); // 刪除最後一個字符
                    }
                } else if (event.text.unicode < 128) { // 只接受 ASCII 字符，需要再改
                    tmp += static_cast<char>(event.text.unicode);
                }
            }
            text.setString(tmp);
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    setgamestatus(1);
                    char* command = "enter_room ";
                    strcat(command, tmp.c_str());
                    strcat(command, "\n");
                    Write(gui_write, command, sizeof(command));
                    return;
                }
            }
        }
        window.draw(background);
        window.draw(inputBox);
        window.draw(hint_message);
        window.draw(text);
        window.display();
    }
}