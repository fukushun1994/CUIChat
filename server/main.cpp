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

const int QUEUE_LEN = 5;
const int BUFFER_SIZE = 1024;

void readMessages (int clientSock) {
  while (true) {
    // クライアントからの入力をバッファに読み込む
    char buff[BUFFER_SIZE];
    size_t size = read(clientSock, buff, BUFFER_SIZE);
    cout << buff << endl;
    if (size == 0) {
      // EOF
      break;
    }
  }
}

int main () {
  int serverSock;
  struct sockaddr_in serverSockAddr; 
  unsigned short serverPort = 8080; //8080番ポートで待受け

  // TCPソケットの作成
  // socket(プロトコルファミリー, 通信の種類, プロトコルの種類)
  // プロトコルファミリー：IPv4のインターネット・プロトコルの場合 PF_INET
  // 通信の種類：ストリーム型かデータグラム型か．ストリーム型なので SOCK_STREAM
  // プロトコルの種類：TCPを使うので IPPROTO_TCP
  if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    // 作成失敗
    cerr << "socket() failed" << endl;
    exit(EXIT_FAILURE);
  }

  memset(&serverSockAddr, 0, sizeof(serverSockAddr)); //何が入っているかわからないので，安全のためゼロクリア
  serverSockAddr.sin_family = AF_INET;  // IPv4を表す
  // htonl, htons はホストバイトオーダーをネットワークバイトオーダーに変換する
  // htonl は long型に， htons は short型に変換する
  serverSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY を指定するとサーバが複数のNICをもつ場合でも，どのアドレスでも接続を待ち受けられる
  serverSockAddr.sin_port = htons(serverPort);

  // 待受けの設定
  // bind(ソケット, ソケットアドレス, ソケットアドレスのサイズ)
  // ソケットアドレスは sockaddr構造体のポインタへのキャストが必要
  if (bind(serverSock, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) < 0) {
    // bind失敗
    cerr << "bind() failed" << endl;
    exit(EXIT_FAILURE);
  }

  // 待受開始
  // listen(ソケット, 保留中の接続のキューの最大長)
  if (listen(serverSock, QUEUE_LEN) < 0) {
    cerr << "listen() failed" << endl;
    exit(EXIT_FAILURE);
  } 

  int clientSock;
  struct sockaddr_in clientSockAddr;
  unsigned int clientLen;

  cout << "listening..." << endl;

  clientLen = sizeof(clientSockAddr);
  // 接続が来たら，クライアントと通信を行うためのソケットを取得する
  // clientSockAddrとclientLenにはクライアントの情報が格納される
  if ((clientSock = accept(serverSock, (struct sockaddr*)&clientSockAddr, &clientLen)) < 0) {
    cerr << "accept() failed" << endl;
    exit(EXIT_FAILURE);
  }
  cout << "connected from " << inet_ntoa(clientSockAddr.sin_addr) << endl;
  
  bool isFinished = false;
  // 読み込み用のスレッドを作成・開始
  thread readThread(
    [&]{
      readMessages(clientSock);
      isFinished = true;
    });
    
  while(!isFinished) {
    char input[BUFFER_SIZE];
    int length;

    if (fgets(input, BUFFER_SIZE, stdin) == 0) {
      break;
    }
    length = strlen(input);
    if (input[length - 1] == '\n') {
      input[length - 1] = '\0'; //改行文字をヌル文字に変更
    }
    if (isFinished) {
      break;
    }
    // 入力をクライアントへ送信する
    write(clientSock, input, length);
  }
  close(clientSock);
  return 0;
}