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
struct Packet{
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
unsigned char makeHeader(int packetID,int frameType){
    return ((packetID & 0x0F) << 1) | (frameType & 0x01);
}
int getPacketID(unsigned char header){
    return (header >> 1) & 0x0F;
}
int getFrameType(unsigned char header){
    return header & 0x01;
}
template<class T>
Node<T>* find(Node<T>* root, int id){
    while(root != NULL)
    {
        if(id == root->data.id)
            return root;

        if(id < root->data.id)
            root = root->left;
        else
            root = root->right;
    }
    return NULL;
}
template<class T>
Node<T>* insert(Node<T>*& root, T data){
    Node<T>*newNode=new Node<T>;
    newNode->data=data;
    if(root==NULL)
    {
        root=newNode;
        return newNode;
    }
    Node<T>* temp=root;
    while(true)
    {
        if(data.id < temp->data.id)
        {
            if(temp->left==NULL)
            {
                temp->left=newNode;
                return newNode;
            }
            temp=temp->left;
        }
        else
        {
            if(temp->right==NULL)
            {
                temp->right = newNode;
                return newNode;
            }
            temp = temp->right;
        }
    }
}
bool sendAll(SOCKET sock,char*buffer,int length){
    int total=0;
    while(total < length)
    {
        int n=send(sock,buffer+total,length-total,0);
        if(n <= 0)
            return false;
        total+=n;
    }
    return true;
}
bool recvAll(SOCKET sock,char* buffer,int length){
    int total = 0;
    while(total < length)
    {
        int n=recv(sock,buffer+total,length-total,0);
        if(n <= 0)
            return false;
        total+=n;
    }
    return true;
}
bool sendPacket(SOCKET client,int packetID,int length,int frameType,const void* data){
    if(length<0 || length>255){
        return false;
    }
    unsigned char header =makeHeader(packetID,frameType);
    if(!sendAll(client,(char*)&header,1)){
        return false;
    }
    if(!sendAll(client,(char*)&length,1)){
        return false;
    }
    if(length > 0){
        if(!sendAll(client,(char*)data,length)){
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
void sendResult(SOCKET client,const char* msg){
    int length = strlen(msg);
    if(length > 255)
    length = 255;
    sendPacket(client,PACKET_RESULT,length,FRAME_SINGLE,msg);
}
template<typename T>
void savefile(T&data,unsigned char type){
    FILE*fp=fopen("data.bin","ab");
    if(fp==NULL){
        printf("File open failed\n");
        return;
    }
    fwrite(&type,sizeof(type),1,fp);
    fwrite(&data,sizeof(T),1,fp);
    fclose(fp);
    printf("Data saved to data.bin\n");
}
template<typename T>
void loadvalue(FILE*f,Node<T>*&root){
    T data;
    if(fread(&data,sizeof(T),1,f)==1){
        if(find(root,data.id)== NULL){
            insert(root,data);
        }
    }
}
void loadfile(){
    FILE*f=fopen("data.bin","rb");
    if(f==NULL){
        printf("No old data found!\n");
        return;
    }
    unsigned char type;
    while (fread(&type,sizeof(type),1,f)==1){
        if(type==1){
            loadvalue(f,empRoot);
        }
        else if(type==2){
            loadvalue(f,cusRoot);
        }
        else if(type==3){
            loadvalue(f,saleRoot);
        }
        else{
            printf("Invalid!");
            break;
        }
    }
    fclose(f);
    printf("\nData loaded successfully\n");
}
void sendEmployee(Node<Employee>*root,SOCKET client){
    if(root==NULL){
        return;
    }
    sendEmployee(root->left,client);
    Employee&e=root->data;
    sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&e.id);
    sendPacket(client,PACKET_EMPLOYEE_NAME,strlen(e.empName),FRAME_MORE,e.empName);
    sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&e.salary);
    sendEmployee(root->right,client);
}
void sendCustomer(Node<Customer>*root,SOCKET client){
    if(root==NULL){
        return;
    }
    sendCustomer(root->left,client);
    Customer&c=root->data;
    sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&c.id);
    sendPacket(client,PACKET_CUSTOMER_NAME,strlen(c.cusName),FRAME_MORE,c.cusName);
    sendPacket(client,PACKET_CUSTOMER_ADDRESS,strlen(c.address),FRAME_SINGLE,c.address);
    sendCustomer(root->right,client);
}
void sendSale(Node<Sale>*root,SOCKET client){
    if(root==NULL){
        return;
    }
    sendSale(root->left,client);
    Sale&s=root->data;
    sendPacket(client,PACKET_SALE_ID,sizeof(int),FRAME_MORE,&s.id);
    sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&s.amount);
    sendPacket(client,PACKET_SALE_DATE,strlen(s.date),FRAME_SINGLE,s.date);
    sendSale(root->right,client);
}
void sendViewAll(SOCKET client){
    printf("\nSending all datas\n");
    sendEmployee(empRoot,client);
    sendCustomer(cusRoot,client);
    sendSale(saleRoot,client);
    sendResult(client,"END");
    printf("View All completed\n");
}
void handleAdd(SOCKET client)
{
    Employee emp;
    Customer cus;
    Sale sale;
    memset(&emp, 0, sizeof(emp));
    memset(&cus, 0, sizeof(cus));
    memset(&sale, 0, sizeof(sale));
    bool empID = false;
    bool empName = false;
    bool empSalary = false;
    bool cusID = false;
    bool cusName = false;
    bool cusAddress = false;
    bool saleID = false;
    bool saleAmount=false;
    bool saleDate=false;
    while(true){
        Packet packet;
        if(!receivePacket(client, packet)){
            printf("Client disconnected.\n");
            return;
        }
        int packetID = getPacketID(packet.header);
        int length = packet.length;
        int frameType = getFrameType(packet.header);
        printf("\nReceived Packet");
        printf("\nPacket ID = %d", packetID);
        printf("\nLength = %d", length);
        printf("\nFrame Type = %d\n", frameType);
        if(packetID==PACKET_VIEW_ALL){
            printf("\nView all Request\n");
            sendViewAll(client);
            continue;
        }
        if(packetID == PACKET_EMPLOYEE_ID)
        {
            if(length != sizeof(int))
            {
                sendResult(client, "BAD ID");
                continue;
            }
            memcpy(&emp.id,packet.data,sizeof(int));
            empID = true;
            printf("Employee ID = %d\n",emp.id);
        }
        else if (packetID==PACKET_EMPLOYEE_NAME){
            if(length>=sizeof(emp.empName)){
                sendResult(client,"BAD NAME");
                continue;
            }
            memcpy(emp.empName,packet.data,length);
            emp.empName[length]='\0';
            empName=true;
            printf("Employee Name: %s\n",emp.empName);
        }
        else if(packetID == PACKET_EMPLOYEE_SALARY){
            if(length != sizeof(float)){
                sendResult(client, "BAD SALARY");
                continue;
            }
            memcpy(&emp.salary,packet.data,sizeof(float));
            empSalary = true;
            printf("Employee Salary = %.2f\n",emp.salary);
        }
        else if(packetID == PACKET_CUSTOMER_ID){
            if(length != sizeof(int)){
                sendResult(client, "BAD CUSTOMER ID");
                continue;
            }
            memcpy(&cus.id,packet.data,sizeof(int));
            cusID = true;
            printf("Customer ID = %d\n",cus.id);
        }
        else if(packetID == PACKET_CUSTOMER_NAME){
            if(length >= sizeof(cus.cusName)){
                sendResult(client, "BAD CUSTOMER NAME");
                continue;
            }
            memcpy(cus.cusName,packet.data,length);
            cus.cusName[length] = '\0';
            cusName = true;
            printf("Customer Name = %s\n",cus.cusName);
        }
        else if(packetID == PACKET_CUSTOMER_ADDRESS){
            if(length >= sizeof(cus.address)){
                sendResult(client, "BAD ADDRESS");
                continue;
            }
            memcpy(cus.address,packet.data,length);
            cus.address[length] = '\0';
            cusAddress = true;
            printf("Customer Address = %s\n",cus.address);
        }
        else if(packetID == PACKET_SALE_ID){
            if(length != sizeof(int)){
                sendResult(client, "BAD SALE ID");
                continue;
            }
            memcpy(&sale.id,packet.data,sizeof(int));
            saleID=true;
            printf("Sale Id: %d\n",sale.id);
        }
        else if(packetID==PACKET_SALE_AMOUNT){
            if(length!=sizeof(float)){
                sendResult(client,"BAD SALE AMOUNT");
                continue;
            }
            memcpy(&sale.amount,packet.data,sizeof(float));
            saleAmount=true;
            printf("Sale Amount: %.2f\n",sale.amount);
        }
        else if(packetID==PACKET_SALE_DATE){
            if(length>=sizeof(sale.date)){
                sendResult(client,"BAD SALE DATE");
                continue;
            }
            memcpy(sale.date,packet.data,length);
            sale.date[length]='\0';
            saleDate=true;
            printf("Sale date: %s\n",sale.date);
        }
        if(empID &&empName &&empSalary){
            if(find(empRoot, emp.id) != NULL){
                sendResult(client,"DUPLICATE");
                empID = false;
                empName = false;
                empSalary = false;
                memset(&emp,0,sizeof(emp));
                continue;
            }
            insert(empRoot, emp);
            savefile(emp,1);
            printf("Employee added successfully.\n");
            sendResult(client, "OK");
            empID = false;
            empName = false;
            empSalary = false;
            memset(&emp,0,sizeof(emp));
        }
        if(cusID &&cusName &&cusAddress){
            if(find(cusRoot, cus.id) != NULL){
                sendResult(client,"DUPLICATE");
                cusID = false;
                cusName = false;
                cusAddress = false;
                memset(&cus,0,sizeof(cus));
                continue;
            }
            insert(cusRoot, cus);
            savefile(cus,2);
            printf("Customer added successfully.\n");
            sendResult(client, "OK");
            cusID = false;
            cusName = false;
            cusAddress = false;
            memset(&cus,0,sizeof(cus));
        }
        if(saleID && saleAmount && saleDate){
            if(find(saleRoot,sale.id)!= NULL){
                sendResult(client,"DUPLICATE SALE");
                saleID=false;
                saleAmount=false;
                saleDate=false;
                memset(&sale,0,sizeof(sale));
                continue;
            }
            insert(saleRoot,sale);
            savefile(sale,3);
            printf("Sale added successfully");
            sendResult(client,"OK");
            saleID=false;
            saleAmount=false;
            saleDate=false;
            memset(&sale,0,sizeof(sale));
        }
    }
}
int main(){
    WSADATA wsa;
    loadfile();
    WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET serverSocket =socket(AF_INET,SOCK_STREAM,0);
    if(serverSocket==INVALID_SOCKET){
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =inet_addr("127.0.0.1");
    if(bind(serverSocket,(sockaddr*)&server,sizeof(server))==SOCKET_ERROR){
        printf("Bined failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if(listen(serverSocket, 5)==SOCKET_ERROR){
        printf("Listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Server waiting for Client 1...\n");
    SOCKET client1 =accept(serverSocket,NULL,NULL);
    if(client1==INVALID_SOCKET){
        printf("Client 1 Accept Failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Client 1 connected\n");
    handleAdd(client1);
    closesocket(client1);
    printf("Client 1 disconnected\n");
    printf("Server waiting for Client 2...\n");
    SOCKET client2 =accept(serverSocket,NULL,NULL);
    if(client1==INVALID_SOCKET){
        printf("Client 2 Accept Failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Client 2 connected.\n");
    handleAdd(client2);
    closesocket(client2);
    printf("Client 2 disconnected\n");
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}