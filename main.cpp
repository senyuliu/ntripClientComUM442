#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "ntripclient.h"

#include <termios.h>
#include <unistd.h>

#include <deque>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread/thread.hpp>
#include <mutex>
#include <sstream>
#include <fstream>
#include <iostream>
#include "comm_timer.h"

using namespace std; 

std::string ReadSerial(int _fd, int _maxLineCount) {
    printf("%s start.\n", __FUNCTION__);
    const size_t BUFFER_SIZE = 1000;
    unsigned char buf[BUFFER_SIZE];
    std::string frameBuf("");
    std::string latLonHei("");
    std::string slamProtocol("");
    bool isFirstFrame = true;
    std::fstream file;
    std::fstream filesv;
    std::string imuFileName("initial_name");
    std::string path   = "/home/linaro/projects/rawdata/json_log/";
    std::string pathsv = "/home/linaro/projects/rawdata/log_save/";

    const int MAX_LINE_COUNT = _maxLineCount;
    int lineCount = 0;
    while(1) 
    {
        bzero(buf, BUFFER_SIZE);
        int nread = read(_fd, buf, BUFFER_SIZE);
        if(nread <= 0) {
            continue;
        }

        for(int i = 0; i < nread; ++i) {
            bool isFrameCompleted = false;
            std::string frameCompleted("");
            switch(buf[i]) {
            case '$': {
                isFirstFrame = false;
                frameBuf = buf[i];
                break;
            }
            case '\r':
                break;
            case '\n':
                isFrameCompleted = true;
                frameCompleted = frameBuf;
                frameBuf.clear();
                break;
            default:
                frameBuf += buf[i];
            }

            if(isFirstFrame) {
                continue;
            }

            if(!isFrameCompleted) {
                continue;
            }
            std::cout << "Receive " << frameCompleted << "\n";

            return frameCompleted; 

            lineCount %= MAX_LINE_COUNT;
            if(0 == lineCount) {
                struct timeval now;
                gettimeofday(&now, NULL);
                imuFileName = std::to_string(now.tv_sec + now.tv_usec / 1000000.);
                std::cout<<"log save cnt: "<<lineCount<<std::endl; 
            }
            //imuFileName = path + imuFileName;
            std::string path_name  =" ";
            std::string path_namesv=" ";

            path_name  = path + imuFileName + ".txt";
            path_namesv= pathsv +imuFileName+ ".txt";

            /*---------------------------------------------------
            filesv.open(path_namesv,std::ios::out | std::ios::app);            
            file.open(path_name, std::ios::out | std::ios::app);
            if(!file)
            {
                std::cerr << "Failed to open " << imuFileName << "\n";
                exit(1);
            }
            if(!filesv)
            {
                std::cerr << "Failed to open " << imuFileName << "\n";
                exit(1);
            }
            filesv << frameCompleted << "\n"; 
            file << frameCompleted << "\n";
            -----------------------------------------------------*/
            file.close();
            filesv.close();
            ++lineCount;
        }
    }
}

int SetSerialOption(int fd, int nSpeed, int nBits, char nEvent, int nStop) {
    std::cout << __FUNCTION__ << " start.";
    struct termios newtio, oldtio;
    if(tcgetattr(fd, &oldtio) != 0) {
        std::cerr << ("Setup Serial 1.");
        exit(1);
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= (CLOCAL | CREAD);
    newtio.c_cflag &= ~CSIZE;

    switch(nBits) {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    default:
        std::cerr << "Unsupported data size.";
        exit(1);
    }

    switch(nEvent) {
    case 'o':
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'e':
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'n':
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    default:
        std::cerr << "Unsupported parity.\n";
        exit(1);
    }

    switch(nSpeed) {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 230400:
        cfsetispeed(&newtio, B230400);
        cfsetospeed(&newtio, B230400);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        std::cerr << "nSpeed: " << nSpeed;
        exit(1);
    }

    if(1 == nStop) {
        newtio.c_cflag &= ~CSTOPB;
    }
    else
    if(2 == nStop) {
        newtio.c_cflag |= CSTOPB;
    }
    else {
        std::cerr << "Setup nStop unavailable.";
        exit(1);
    }

    tcflush(fd, TCIFLUSH);

    newtio.c_cc[VTIME] = 100;
    newtio.c_cc[VMIN] = 0;

    if(0 != tcsetattr(fd, TCSANOW, &newtio)) {
        std::cerr << "Com setup error.\n";
        exit(1);
    }

    std::cout << "Set done.\n";
    return 0;
}

int string2int(const std::string& str) {
    std::istringstream iss(str);
    int num;
    iss >> num;
    return num;
}

int main_test(int argc, char **argv) {
    std::cout << "Got " << argc << " parameters.\n";
    int maxLineCount = 20;
    if(argc > 1) {
        const std::string argv1(argv[1]);
        maxLineCount = string2int(argv1);
    }
    std::cout << "maxLineCount " << maxLineCount << "\n";
    int fd = open("/dev/ttyUSB0", O_RDWR);
    if(fd < 0) {
        printf("Failed to open device.\n");
        return -1;
    }

    if(SetSerialOption(fd, 115200, 8, 'N', 1) < 0) {
        printf("Failed to setup.\n");
        return -1;
    }

    // (void)WriteSerial();
    (void)ReadSerial(fd, maxLineCount);
    return 0;

}



//#define GGA_TEST_DATA "$GNGGA,062608.00,4004.3034189,N,11614.3359021,E,4,13,0.85,95.533,M,0.000,M,1,0004*76\r\n"

#define GGA_TEST_DATA "$GPGGA,150825.00,4004.31479970,N,11614.37122897,E,1,10,3.1,81.8245,M,-9.4008,M,,*48\r\n"

typedef enum {
  disconnected,
  connected,
} link_t;

int main(int argc, char *argv[])
{

    std::cout << "Got " << argc << " parameters.\n";
    int maxLineCount = 20;
    if(argc > 1) {
        const std::string argv1(argv[1]);
        maxLineCount = string2int(argv1);
    }
    std::cout << "maxLineCount " << maxLineCount << "\n";
    int fd = open("/dev/ttyUSB0", O_RDWR);
    if(fd < 0) {
        printf("Failed to open device.\n");
        return -1;
    }

    if(SetSerialOption(fd, 115200, 8, 'N', 1) < 0) {
        printf("Failed to setup.\n");
        return -1;
    }

    // (void)WriteSerial();

  struct timeval tv;
  ntrip_client_t ntrip_client;
  char gga_buf[200]   = {0};
  char recv_buf[4096] = {0};
  int send_len = 0;
  int recv_len = 0;
  link_t ntrip_link = disconnected;
  fd_set rdfds;
  int maxfd   = 0;
  int timeout = 1;
  int ret     = 0;
  int i       = 0;
	
  memset(ntrip_client.dst_host, 0, sizeof(ntrip_client.dst_host));
  memset(ntrip_client.dst_port, 0, sizeof(ntrip_client.dst_port));
  memset(ntrip_client.dst_mount, 0, sizeof(ntrip_client.dst_mount));
  memset(ntrip_client.dst_user, 0, sizeof(ntrip_client.dst_user));
  memset(ntrip_client.dst_password, 0, sizeof(ntrip_client.dst_password));

  argv[1] = "116.213.73.211";
  argv[2] = "17779";
  argv[3] = "RTCM32_GNSS";
  argv[4] = "zhangxu";
  argv[5] = "zhangxu";
	
  strcpy(ntrip_client.dst_host, argv[1]);
  strcpy(ntrip_client.dst_port, argv[2]);
  strcpy(ntrip_client.dst_mount, argv[3]);
  strcpy(ntrip_client.dst_user, argv[4]);
  strcpy(ntrip_client.dst_password, argv[5]);
	
  while(1)
  {
    if(ntrip_link != connected)
    {
      ret = ntripclient_connect(&ntrip_client);
      if(ret == 1)
      {
        ntrip_link = connected;
      }
    }	
    std::string GPGGA = ReadSerial(fd, maxLineCount);
    GPGGA = GPGGA + "\r\n";

    if(ntrip_link == connected)
    {
      memset(gga_buf, 0, sizeof(gga_buf));
      strcpy(gga_buf, GPGGA.c_str() );//GGA_TEST_DATA); //GPGGA.c_str() );
      send_len = strlen(gga_buf);
      //printf("Send: %s", gga_buf);

      if(ntripclient_send(gga_buf, send_len) <= 0)
      {
        ntripclient_close();
        ntrip_link = disconnected;
        continue;
      }
		
      maxfd = sockfd_ntripc + 1;
      FD_ZERO(&rdfds);
      FD_SET(sockfd_ntripc, &rdfds);
		
      tv.tv_sec = timeout;
      tv.tv_usec = 0;
		
      ret = select(maxfd, &rdfds, NULL, NULL, &tv);
      if(ret < 0)
      {
        printf("select error (%s)\n", strerror(errno)); 
        continue;
      }
      else if(ret == 0)
      {
        continue;
      }
      else
      {
        if(FD_ISSET(sockfd_ntripc, &rdfds))
	{
          recv_len = ntripclient_receive(recv_buf, sizeof(recv_buf));
          if(recv_len > 0)
          {
            printf("Recv: %d bytes\n{", recv_len);
	    for(i = 0; i < recv_len; i++)
	    {
              /*if((i % 50) == 0)
              {
                printf("\n");
              }*/
              printf("%02X ",(unsigned char)recv_buf[i]);
              write(fd, &recv_buf[i], 1);
            }
            printf("\n}\n");
          }
        }
      }		
    }

    sleep(1);
  }

  ntripclient_close();
  ntrip_link = disconnected;

  return 1;
}
