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
#include <sys/select.h>
#include <vector>
#include <algorithm>

using namespace std;

const int QUEUE_LEN = 5;
const int BUFFER_SIZE = 1024;

// クライアントからのメッセージをバッファに読み込む
size_t readMessage (int sock, char *buff) {
  size_t size = read(sock, buff, BUFFER_SIZE);
  return size;
}

// 全クライアントにメッセージを送信する
void broadcast(vector<int>& clientSockets, char *buff) {
  vector<int>::iterator it = clientSockets.begin();
  for (int sock : clientSockets) {
    ssize_t result = write(sock, buff, strlen(buff)+1); // NULL文字分+1
    if (result < -1) {
      cerr << "error occurered in write() to " << sock << endl;
      exit(EXIT_FAILURE);
    } 
  }
}

int listen_socket(unsigned short port) {
  int serverSock;
  struct sockaddr_in serverSockAddr; 
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
  serverSockAddr.sin_port = htons(port);

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
  return serverSock;
}

int acceptClient(int serverSocket) {
  int clientSock;
  struct sockaddr_in clientSockAddr;
  unsigned int clientLen = sizeof(clientSockAddr);
  // 接続が来たら，クライアントと通信を行うためのソケットを取得する
  // clientSockAddrとclientLenにはクライアントの情報が格納される
  if ((clientSock = accept(serverSocket, (struct sockaddr*)&clientSockAddr, &clientLen)) < 0) {
    cerr << "accept() failed" << endl;
    exit(EXIT_FAILURE);
  }
  cout << "connected from " << inet_ntoa(clientSockAddr.sin_addr) << endl;
  return clientSock;
}

void closeClient(int clientSocket, fd_set *rfds) {
  close(clientSocket);
  FD_CLR(clientSocket, rfds);
}

// 読み込み用のファイルディスクリプタの集合を初期化し，最大のソケットの値を返す
int initializeFD(fd_set *rfds, int serverSocket, vector<int>& clientSockets) {
  int maxFds = serverSocket;
  FD_ZERO(rfds);
  FD_SET(serverSocket, rfds);

  for (int sock : clientSockets) {
    FD_SET(sock, rfds);
    maxFds = max(maxFds, sock);
  }
  return maxFds;
}

int main () {
  unsigned short serverPort = 8080; //8080番ポートで待受け
  vector<int> clientSockets = {};

  int serverSock = listen_socket(serverPort); // 待受開始

  while(true) {
    fd_set rfds;
    // ハマりポイント！！！ファイルディスクリプタ集合は毎回初期化しないといけない！
    int maxFds = initializeFD(&rfds, serverSock, clientSockets);
    int result = select(maxFds+1, &rfds, NULL, NULL, NULL); // なにかイベントが発生するまで待つ
    if (result == -1) {
      // エラー
      cerr << "error occured in select()" << endl;
      break;
    }
    if (FD_ISSET(serverSock, &rfds)) {
      // 新規接続
      int sock = acceptClient(serverSock);
      clientSockets.push_back(sock);
    } else {
      // 各クライアントからの書き込み
      vector<int>::iterator it = clientSockets.begin();
      while (it != clientSockets.end()) {
        int sock = *it;
        if (FD_ISSET(sock, &rfds)) {
          char buff[BUFFER_SIZE];
          size_t size = readMessage(sock, buff);
          if (size == 0) {
            close(sock);
            it = clientSockets.erase(it);
            cout << "close " << sock << endl;
            continue; 
          } 
          cout << "message from " << sock << ": " << buff << endl;
          broadcast(clientSockets, buff);
        }
        it++;
      }
    }
  }
  for (int sock : clientSockets) {
    if (sock >= 0) {
      close(sock);
    }
  }
  
  return 0;
}