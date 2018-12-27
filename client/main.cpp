#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

const int BUFFER_SIZE = 1024;

void readMessages (int socket) {
  while (true) {
    // サーバからのメッセージをバッファに読み込む
    char buff[BUFFER_SIZE];
    size_t size = read(socket, buff, BUFFER_SIZE);
    if (size == 0) {
      // EOF
      break;
    }
    cout << buff << endl;
  }
}

void writeMessages (int socket) {
  while (true) {
    char input[BUFFER_SIZE];
    int length;

    if (fgets(input, BUFFER_SIZE, stdin) == 0) {
      break;
    }
    length = strlen(input);
    if (input[length - 1] == '\n') {
      input[length - 1] = '\0'; //改行文字をヌル文字に変更
    }
    write(socket, input, length);
  }
}

int main () {
  int sock;
  struct sockaddr_in serverSockAddr; // サーバへの接続情報
  unsigned short serverPort = 8080; //8080番ポートに接続
  char* serverAddress = "172.17.0.2"; //サーバのIPアドレス

  // TCPソケットの作成
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    // 作成失敗
    cerr << "socket() failed" << endl;
    exit(EXIT_FAILURE);
  }

  memset(&serverSockAddr, 0, sizeof(serverSockAddr)); //何が入っているかわからないので，安全のためゼロクリア
  serverSockAddr.sin_family = AF_INET;  // IPv4を表す
  // inet_atonでIPアドレスをネットワークバイトオーダーのバイナリ表現に変換し，serverSockAddr.sin_addrに格納
  if (inet_aton(serverAddress, &serverSockAddr.sin_addr) == 0) {
    cerr << "Invalid IP Address." << endl;
    exit(EXIT_FAILURE);
  }
  serverSockAddr.sin_port = htons(serverPort);

  // サーバへ接続
  if (connect(sock, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) < 0) {
    cerr << "connect() failed" << endl;
    exit(EXIT_FAILURE);
  }
  cout << "connected to " << inet_ntoa(serverSockAddr.sin_addr) << endl;

  bool isFinished = false;
  // 読み込み用のスレッドを作成・開始
  thread readThread(
    [&]{
      readMessages(sock);
      isFinished = true;
    });
  thread writeThread(
    [&]{
      writeMessages(sock);
      isFinished = true;
    });

   while(!isFinished) {
    // main loop
  }
  readThread.detach();
  writeThread.detach();
  close(sock);
  return 0;
}