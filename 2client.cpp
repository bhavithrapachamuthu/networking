#include <stdio.h>
#include <WinSock2.h>
#include <cstring>
#pragma comment(lib,"ws2_32.lib")
#define PORT 8080
#define PACKET_EMPLOYEE_ID 1
#define PACKET_EMPLOYEE_NAME 2
#define PACKET_EMPLOYEE_SALARY 3
#define PACKET_CUSTOMER_ID 4
#define PACKET_CUSTOMER_NAME 5
#define PACKET_CUSTOMER_ADDRESS 6
#define PACKET_SALE_ID 7
#define PACKET_SALE_AMOUNT 8
#define PACKET_SALE_DATE 9
#define PACKET_RESULT 10
#define PACKET_VIEW_ALL 11
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet
{
    unsigned char header;
    unsigned char length;
    unsigned char data[100000];
};
class Data{
public:
    int id;
};
class Employee : public Data{
public:
    char empName[50];
    float salary;
};
class Customer : public Data{
public:
    char cusName[50];
    char address[100];
};
class Sale : public Data{
public:
    float amount;
    char date[20];
};
template<class T>
struct Node{
    T data;
    Node<T>* left;
    Node<T>* right;

    Node()
    {
        left = NULL;
        right = NULL;
    }
};
Node<Employee>* empRoot = NULL;
Node<Customer>* cusRoot = NULL;
Node<Sale>* saleRoot = NULL;
unsigned char makeHeader(int packetID,int length,int frameType){
    return ((packetID & 0x0F) << 1) |(frameType & 0x01);
}
int getPacketID(unsigned char header){
    return (header >> 1) & 0x0F;
}
int getFrameType(unsigned char header){
    return header & 0x01;
}
bool sendAll(SOCKET client,const char*buffer,int length){
    int total = 0;
    while(total < length){
        int n = send(client,buffer + total,length - total,0);
        if(n <= 0)
            return false;
        total += n;
    }
    return true;
}
bool recvAll(SOCKET client,char* buffer,int length)
{
    int total = 0;
    while(total < length){
        int n = recv(client,buffer + total,length - total, 0);
        if(n <= 0)
            return false;
        total += n;
    }
    return true;
}
bool sendPacket(SOCKET client,int packetID,int length,int frameType,const void* data)
{
    if(length<0 || length>255){
        return false;
    }
    unsigned char header =makeHeader(packetID,length,frameType);
    if(!sendAll(client,(char*)&header,1)){
        return false;
    }
    if(!sendAll(client,(char*)&length,1)){
        return false;
    }
    if(length > 0){
        if(!sendAll(client,(const char*)data,length)){
            return false;
        }
    }
    return true;
}
bool receivePacket(SOCKET client,Packet& packet){
    if(!recvAll(client,(char*)&packet.header,1))
    {
        return false;
    }
    if(!recvAll(client,(char*)&packet.length,1)){
        return false;
    }
    if(packet.length > 0){
        if(!recvAll(client,(char*)packet.data,packet.length)){
            return false;
        }
    }
    return true;
}
bool receiveResult(SOCKET client){
    Packet packet;
    if(!receivePacket(client,packet)){
        return false;
    }
    int packetID =getPacketID(packet.header);
    if(packetID!=PACKET_RESULT){
        return false;
    }
    char result[256];
    int len=packet.length;
    if(len>255)
    len=255;
    memcpy(result,packet.data,len);
    result[packet.length]='\0';
    printf("\nServer: %s\n",result);
    return true;
}
void addEmployee(SOCKET client)
{
    int id;
    char name[50];
    float salary;
    printf("\nEnter Employee ID : ");
    scanf("%d", &id);
    getchar();
    printf("Enter Employee Name : ");
    fgets(name,sizeof(name),stdin);
    name[strcspn(name,"\n")]='\0';
    printf("Enter Employee Salary : ");
    scanf("%f", &salary);
    sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&id);
    sendPacket(client,PACKET_EMPLOYEE_NAME,strlen(name),FRAME_MORE,name);
    sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&salary);
    receiveResult(client);
}
void addCustomer(SOCKET client){
    int id;
    char name[50];
    char address[100];
    printf("\nEnter Customer ID : ");
    scanf("%d", &id);
    getchar();
    printf("Enter Customer Name : ");
    fgets(name,sizeof(name),stdin);
    name[strcspn(name,"\n")]='\0';
    printf("Enter Customer Address : ");
    fgets(address,sizeof(address),stdin);
    address[strcspn(address,"\n")]='\0';
    sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&id);
    sendPacket(client,PACKET_CUSTOMER_NAME,strlen(name),FRAME_MORE,name);
    sendPacket(client,PACKET_CUSTOMER_ADDRESS,strlen(address),FRAME_SINGLE,address);
    receiveResult(client);
}
void addSale(SOCKET client){
    int id;
    float amount;
    char date[20];
    printf("\nEnter Sale ID : ");
    scanf("%d", &id);
    printf("Enter Sale Amount : ");
    scanf("%f", &amount);
    getchar();
    printf("Enter Sale Date : ");
    fgets(date,sizeof(date),stdin);
    date[strcspn(date,"\n")]='\0';
    sendPacket(client,PACKET_SALE_ID,sizeof(int),FRAME_MORE,&id);
    sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&amount);
    sendPacket(client,PACKET_SALE_DATE,strlen(date),FRAME_SINGLE,date);
    receiveResult(client);
}
void viewAll(SOCKET client)
{
    // View request
    sendPacket(client,PACKET_VIEW_ALL,0,FRAME_SINGLE,NULL);
    while(true)
    {
        Packet packet;
        if(!receivePacket(client,packet)){
            printf("\nServer disconnected\n");
            return;
        }
        int packetID =getPacketID(packet.header);
        int frameType =getFrameType(packet.header);
        int length=packet.length;
        if(packetID == PACKET_RESULT){
            char result[16];
            memcpy(result,packet.data,length);
            result[length]='\0';
            if(strcmp(result,"END") == 0){
                printf("\n--- VIEW END ---\n");
                break;
            }
            printf("\nServer : %s\n",packet.data);
            continue;
        }
        if(packetID == PACKET_EMPLOYEE_ID)
        {
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("EMPLOYEE\n");
            printf("ID     : %d\n", id);
        }
        else if(packetID==PACKET_EMPLOYEE_NAME){
            char name[50];
            memcpy(name,packet.data,length);
            name[length]='\0';
            printf("Name   : %s\n", name);
        }
        else if(packetID==PACKET_EMPLOYEE_SALARY){
            float salary;
            memcpy(&salary,packet.data,sizeof(float));
            printf("Salary : %.2f\n", salary);
        }
        else if(packetID == PACKET_CUSTOMER_ID){
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("CUSTOMER\n");
            printf("ID : %d\n", id);
        }
        else if(packetID == PACKET_CUSTOMER_NAME){
            char name[50];
            memcpy(name,packet.data,length);
            name[length]='\0';
            printf("Name: %s\n", name);
        }
        else if(packetID==PACKET_CUSTOMER_ADDRESS){
            char address[100];
            memcpy(address,packet.data,length);
            address[length]='\0';
            printf("Address: %s\n", address);
        }
        else if(packetID == PACKET_SALE_ID){
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("SALE\n");
            printf("ID: %d\n", id);
        }
        else if(packetID == PACKET_SALE_AMOUNT){
            float amount;
            memcpy(&amount,packet.data,sizeof(float));
            printf("Amount: %.2f\n", amount);
        }
        else if(packetID == PACKET_SALE_DATE){
            char date[20];
            int len=length;
            if(len>19)
            len=19;
            memcpy(date,packet.data,length);
            packet.data[length] = '\0';
            printf("Date: %s\n",packet.data);
        }
    }
}
int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET client =socket(AF_INET,SOCK_STREAM,0);
    if(client == INVALID_SOCKET){
        printf("Socket creation failed\n");
        return 1;
    }
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port =htons(PORT);
    server.sin_addr.s_addr =inet_addr("127.0.0.1");
    if(connect(client,(sockaddr*)&server,sizeof(server)) == SOCKET_ERROR)
    {
        printf("Connection failed\n");
        return 1;
    }
    printf("Client 2 Connected to Server\n");
    int choice;
    while(true)
    {
        printf("\n========================\n");
        printf("1. Add Employee\n");
        printf("2. Add Customer\n");
        printf("3. Add Sale\n");
        printf("4. View all\n");
        printf("5. Exit\n");
        printf("========================\n");
        printf("Enter Choice : ");
        scanf("%d", &choice);
        if(choice == 1){
            addEmployee(client);
        }
        else if(choice == 2){
            addCustomer(client);
        }
        else if(choice == 3){
            addSale(client);
        }
        else if(choice == 4){
            viewAll(client);
        }
        else if(choice == 5){
            break;
        }
        else{
            printf("Invalid Choice\n");
        }
    }
    closesocket(client);
    WSACleanup();
    return 0;
}