#include <stdio.h>
#include <WinSock2.h>
#include <string.h>
#include<time.h>
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
#define PACKET_SALE_EMP_ID 10
#define PACKET_SALE_CUS_ID 11
#define PACKET_RESULT 12
#define PACKET_VIEW_ALL 13
#define PACKET_DELETE 14
#define PACKET_SEARCH 15
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet{
    unsigned char header;
    unsigned char length;
    unsigned char data[255];
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
    int empId;
    int cusId;
    float amount;
    char date[26];
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
SOCKET clients[100];
int clientCount=0;
int nextSaleId=1;
unsigned char makeHeader(int packetID,int frameType){
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
    if(length<0 || length>254){
        return false;
    }
    unsigned char header =makeHeader(packetID,frameType);
    unsigned char len=(unsigned char)length;
    if(!sendAll(client,(char*)&header,1)){
        return false;
    }
    if(!sendAll(client,(char*)&len,1)){
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
    memset(&packet,0,sizeof(packet));
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
        packet.data[packet.length]='\0';
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
void addEmployee(SOCKET client){
    int id;
    char name[50];
    float salary;
    while(1){
        printf("\nEnter Employee ID : ");
        if(scanf("%d", &id) != 1)
        {
            printf("Invalid ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&id);
        Packet p;
        if(!receivePacket(client, p))
        {
            printf("Server disconnected\n");
            return;
        }
        p.data[p.length] = '\0';
        if(strcmp((char*)p.data, "DUPLICATE ID") == 0){
            printf("\nEmployee ID already exists!Try different Id.\n");
            continue;
        }
        if(strcmp((char*)p.data, "ID AVAILABLE") == 0){
            break;
        }
        printf("Server: %s\n", p.data);
        return;
    }
    while(getchar() != '\n');
    printf("Enter Employee Name : ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    sendPacket(client,PACKET_EMPLOYEE_NAME,strlen(name),FRAME_MORE,name);
    while(1){
        printf("Enter Employee Salary : ");
        if(scanf("%f", &salary) != 1){
            printf("Invalid Salary. Enter numbers only.\n");
            while(getchar() != '\n');
            continue;
        }
        break;
    }
    sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&salary);
    Packet result;
    if(!receivePacket(client, result)){
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "EMPLOYEE ADDED") == 0){
        printf("\nEmployee added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
void addCustomer(SOCKET client)
{
    int id;
    char name[50];
    char address[100];
    while(1){
        printf("\nEnter Customer ID : ");
        if(scanf("%d", &id) != 1){
            printf("Invalid ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&id);
        Packet p;
        if(!receivePacket(client, p)){
            printf("Server disconnected\n");
            return;
        }
        p.data[p.length] = '\0';
        if(strcmp((char*)p.data, "DUPLICATE ID") == 0){
            printf("\nCustomer ID already exists!Try different Id.\n");
            continue;
        }
        if(strcmp((char*)p.data, "ID AVAILABLE") == 0){
            break;
        }
        printf("Server: %s\n", p.data);
        return;
    }
    while(getchar() != '\n');
    printf("Enter Customer Name : ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    sendPacket(client,PACKET_CUSTOMER_NAME,strlen(name),FRAME_MORE,name);
    printf("Enter Customer Address : ");
    fgets(address, sizeof(address), stdin);
    address[strcspn(address, "\n")] = '\0';
    sendPacket(client,PACKET_CUSTOMER_ADDRESS,strlen(address),FRAME_SINGLE,address);
    Packet result;
    if(!receivePacket(client, result)){
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "CUSTOMER ADDED") == 0){
        printf("\nCustomer added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
void addSale(SOCKET client){
    int saleId;
    int empId;
    int cusId;
    float amount;
    char date[26];
    if(!sendPacket(client,PACKET_SALE_ID,0,FRAME_MORE,NULL)){
        printf("Failed to request Sale Id\n");
        return;
    }
    Packet p;
    if(!receivePacket(client,p)){
        printf("Server disconnected\n");
        return;
    }
    if(getPacketID(p.header)!=PACKET_SALE_ID || p.length!=sizeof(int) ){
        printf("Invalid Sale Id\n");
        return;
    }
    memcpy(&saleId,p.data,sizeof(int));
    printf("\nGenerated Sale Id: %d\n",saleId);
    while(1){
        printf("Enter Employee ID : ");
        if(scanf("%d", &empId) != 1){
            printf("Invalid Employee ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        if(!sendPacket(client,PACKET_SALE_EMP_ID,sizeof(int),FRAME_MORE,&empId)){
            printf("Server disconnected\n");
            return;
        }
        if(!receivePacket(client,p)){
            printf("Server disconnected\n");
            return;
        }
        if(p.length > 0)
        p.data[p.length]='\0';
        if(strcmp((char*)p.data,"EMPLOYEE ID VALID")==0){
            printf("Employee ID valid\n");
            break;
        }
        printf("Server: %s\n",(char*)p.data);
    }
    while(1){
        printf("Enter Customer ID : ");
        if(scanf("%d", &cusId) != 1){
            printf("Invalid Customer ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        if(!sendPacket(client,PACKET_SALE_CUS_ID,sizeof(int),FRAME_MORE,&cusId)){
            printf("Server disconnected\n");
            return;
        }
        if(!receivePacket(client,p)){
            printf("Server disconnected\n");
            return;
        }
        if(p.length<sizeof(p.data))
        p.data[p.length]='\0';
        if(strcmp((char*)p.data,"CUSTOMER ID VALID")==0){
            printf("Customer ID valid\n");
            break;
        }
        printf("Server: %s\n",(char*)p.data);
    }
    while(1){
        printf("Enter Sale Amount : ");
        if(scanf("%f", &amount) != 1){
            printf("Invalid Amount. Enter numbers only.\n");
            while(getchar() != '\n');
            continue;
        }
        break;
    }
    sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&amount);
    while(getchar() != '\n');
    time_t now;
    time(&now);
    strcpy(date, ctime(&now));
    date[strcspn(date, "\n")] = '\0';
    printf("Sale Date : %s\n", date);
    sendPacket(client,PACKET_SALE_DATE,strlen(date),FRAME_SINGLE,date);
    Packet result;
    if(!receivePacket(client, result))
    {
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "SALE ADDED") == 0){
        printf("\nSale added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
void displayEmp(SOCKET client){
    printf("\n========EMPLOYEE DETAILS========\n");
    printf("%-10s %-20s %-15s\n","ID","EMPLOYEE NAME","SALARY");
    printf("----------------------------------------------------\n");
    while(true){
        Packet p;
        if(!receivePacket(client,p)){
            printf("\nServer disconnected\n");
            return;
        }
        int packetID =getPacketID(p.header);
        if(packetID==PACKET_RESULT){
            if(p.length == 3 && memcmp(p.data,"END",3) == 0){
                printf("Emp display completed\n");
                return;
            }
            p.data[p.length]='\0';
            printf("%s\n",p.data);
        }
        else if(packetID == PACKET_EMPLOYEE_ID){
            int id;
            memcpy(&id,p.data,sizeof(int));
            printf("%-10d ",id);
        }
        else if(packetID==PACKET_EMPLOYEE_NAME){
            char name[50];
            int len=p.length;
            if(len>=sizeof(name))
            len=sizeof(name)-1;
            memcpy(name,p.data,len);
            name[len]='\0';
            printf("%-20s ",name);
        }
        else if(packetID==PACKET_EMPLOYEE_SALARY){
            float salary;
            memcpy(&salary,p.data,sizeof(float));
            printf("%-15.2f\n",salary);
        }
        else{
            printf("Unknown Packet Id: %d\n",packetID);
        }
    }
    printf("----------------------------------------------------\n");
}
void displayCus(SOCKET client){
    printf("\n========CUSTOMER DETAILS========\n");
    printf("%-10s %-20s %-20s\n","ID","CUSTOMER NAME","ADDRESS");
    printf("----------------------------------------------------\n");
    while(true){
        Packet p;
        if(!receivePacket(client,p)){
            printf("\nServer disconnected\n");
            return;
        }
        int packetID =getPacketID(p.header);
        if(packetID==PACKET_RESULT){
            if(p.length ==3 && memcmp(p.data,"END",3) == 0){
                printf("Cus display finish\n");
                return;
            }
            p.data[p.length]='\0';
            printf("%s\n",p.data);
        }
        else if(packetID == PACKET_CUSTOMER_ID){
            int id;
            memcpy(&id,p.data,sizeof(int));
            printf("%-10d ",id);
        }
        else if(packetID==PACKET_CUSTOMER_NAME){
            char name[50];
            int len=p.length;
            if(len>=sizeof(name))
            len=sizeof(name)-1;
            memcpy(name,p.data,len);
            name[len]='\0';
            printf("%-20s ",name);
        }
        else if(packetID==PACKET_CUSTOMER_ADDRESS){
            char address[100];
            int len=p.length;
            if(len>=sizeof(address))
            len=sizeof(address)-1;
            memcpy(address,p.data,len);
            address[len]='\0';
            printf("%-20s\n",address);
        }
        else{
            printf("Unknown Packet Id: %d\n",packetID);
        }
    }
    printf("\n----------------------------------------------------\n");
}
void displaySale(SOCKET client){
    printf("\n========SALE DETAILS========\n");
    printf("%-10s %-12s %-12s %-12s %-15s\n","SALE.ID","EMP.ID","CUS.ID","AMOUNT","DATE");
    printf("----------------------------------------------------\n");
    while(true){
        Packet p;
        if(!receivePacket(client,p)){
            printf("\nServer disconnected\n");
            return;
        }
        int packetID =getPacketID(p.header);
        if(packetID==PACKET_RESULT){
            if(p.length == 3 && memcmp(p.data,"END",3) == 0){
                printf("Sale display finish\n");
                return;
            }
            p.data[p.length]='\0';
            printf("%s\n",p.data);
        }
        else if(packetID== PACKET_SALE_ID){
            int id;
            memcpy(&id,p.data,sizeof(int));
            printf("%-10d ",id);
        }
        else if(packetID== PACKET_SALE_EMP_ID){
            int empId;
            memcpy(&empId,p.data,sizeof(int));
            printf("%-12d ",empId);
        }
        else if(packetID == PACKET_SALE_CUS_ID){
            int cusId;
            memcpy(&cusId,p.data,sizeof(int));
            printf("%-12d ",cusId);
        }
        else if(packetID==PACKET_SALE_AMOUNT){
            float amount;
            memcpy(&amount,p.data,sizeof(float));
            printf("%-12.2f ",amount);
        }
        else if(packetID==PACKET_SALE_DATE){
            char date[50];
            int len=p.length;
            if(len>=sizeof(date))
            len=sizeof(date)-1;
            memcpy(date,p.data,len);
            date[len]='\0';
            printf("%-15s\n",date);
        }
        else{
            printf("Unknown Packet Id: %d\n",packetID);
        }
    }
    printf("\n----------------------------------------------------\n");
}
void viewAll(SOCKET client){
    int type;
    printf("\n1. Employee");
    printf("\n2. Customer");
    printf("\n3. Sale");
    printf("\nEnter View Type: ");
    scanf("%d",&type);
    if(type<1 || type>3){
        printf("Invalid Type\n");
        return;
    }
    unsigned char t=(unsigned char)type;
    if(!(sendPacket(client,PACKET_VIEW_ALL,1,FRAME_SINGLE,&t))){
        printf("Server Disconnected\n");
        return;
    }
    if(type==1){
        displayEmp(client);
    }
    else if(type==2){
        displayCus(client);
    }
    else if(type==3){
        displaySale(client);
    }
    else{
        printf("Invalid type\n");
    }
}
void deleteEmp(SOCKET client){
    int id;
    printf("\n===========================\n");
    printf("      DELETE EMPLOYEEE\n");
    printf("\n===========================\n");
    printf("Enter Employee Id: ");
    scanf("%d",&id);
    unsigned char data[5];
    data[0]=1;
    memcpy(data+1,&id,sizeof(int));
    sendPacket(client,PACKET_DELETE,5,FRAME_SINGLE,data);
    Packet p;
    if(!receivePacket(client,p)){
        printf("Server disconnected\n");
        return;
    }
    if(getPacketID(p.header)==PACKET_RESULT){
        char result[256];
        memcpy(result,p.data,p.length);
        result[p.length]='\0';
        printf("\nServer : %s\n",result);
    }
}
void deleteCus(SOCKET client){
    int id;
    printf("\n===========================\n");
    printf("      DELETE CUSTOMER\n");
    printf("\n===========================\n");
    printf("Enter Customer Id: ",id);
    scanf("%d",&id);
    unsigned char data[5];
    data[0]=2;
    memcpy(data+1,&id,sizeof(int));
    sendPacket(client,PACKET_DELETE,5,FRAME_SINGLE,data);
    Packet p;
    if(!receivePacket(client,p)){
        printf("Server disconnected\n");
        return;
    }
    if(getPacketID(p.header)==PACKET_RESULT){
        char result[256];
        memcpy(result,p.data,p.length);
        result[p.length]='\0';
        printf("\nServer : %s\n",result);
    }
}
void deleteSale(SOCKET client){
    int id;
    printf("\n===========================\n");
    printf("      SALE EMPLOYEEE\n");
    printf("\n===========================\n");
    printf("Enter Employee Id: ",id);
    scanf("%d",&id);
    unsigned char data[5];
    data[0]=3;
    memcpy(data+1,&id,sizeof(int));
    sendPacket(client,PACKET_DELETE,5,FRAME_SINGLE,data);
    Packet p;
    if(!receivePacket(client,p)){
        printf("Server disconnected\n");
        return;
    }
    if(getPacketID(p.header)==PACKET_RESULT){
        char result[256];
        memcpy(result,p.data,p.length);
        result[p.length]='\0';
        printf("\nServer : %s\n",result);
    }
}
void deleteData(SOCKET client){
    int type;
    printf("\n1. Employee");
    printf("\n2. Customer");
    printf("\n3. Sale");
    printf("\nEnter Delete Type: ");
    scanf("%d",&type);
    if(type==1){
        deleteEmp(client);
    }
    else if(type==2){
        deleteCus(client);
    }
    else if(type==3){
        deleteSale(client);
    }
    else{
        printf("Invalid type\n");
    }
}
void searchEmp(SOCKET client){
    char name[50];
    printf("\n=========================================\n");
    printf("\n         SEARCH EMPLOYEE\n");
    printf("\n=========================================\n");
    printf("Enter Employee Name: ");
    scanf("%49s",name);
    sendPacket(client,PACKET_SEARCH,strlen(name),FRAME_SINGLE,name);
    printf("\nSearching...\n");
    while(1){
        Packet p;
        if(!receivePacket(client,p)){
            printf("Server disconnected\n");
            return;
        }
        int packetId=getPacketID(p.header);
        if(packetId==PACKET_RESULT){
            char result[256];
            memcpy(result,p.data,p.length);
            result[p.length]='\0';
            if(strcmp(result,"EMPLOYEE NOT FOUND")==0){
                printf("\nEmployee not Found!\n");
                return;
            }
                if(strcmp(result,"END")==0){
                    printf("\nSEARCH COMPLETED\n");
                    break;
                }
                printf("\nServer :%s\n",result);
        }
        else if(packetId==PACKET_EMPLOYEE_ID){
            int id;
            memcpy(&id,p.data,sizeof(int));
            printf("\nEmployee Id: %d\n",id);
        }
        else if(packetId==PACKET_EMPLOYEE_NAME){
            char name[50];
            memcpy(name,p.data,p.length);
            name[p.length]='\0';
            printf("\nEmployee Name: %s\n",name);
        }
        else if(packetId==PACKET_EMPLOYEE_SALARY){
            float salary;
            memcpy(&salary,p.data,sizeof(float));
            printf("\nEmployee Salary: %.2f\n",salary);
        }
        else if(packetId==PACKET_SALE_ID){
            int saleId;
            memcpy(&saleId,p.data,sizeof(int));
            printf("\n-------SALE-------\n");
            printf("\nSale Id: %d\n",saleId);
        }
        else if(packetId == PACKET_SALE_EMP_ID){
            int id;
            memcpy(&id,p.data,sizeof(int));
            printf("ID: %d\n", id);
        }
        else if(packetId == PACKET_SALE_CUS_ID){
            int cusId;
            memcpy(&cusId,p.data,sizeof(int));
            printf("Customer Id: %d\n",cusId);
        }
        else if(packetId == PACKET_SALE_AMOUNT){
            float amount;
            memcpy(&amount,p.data,sizeof(float));
            printf("Amount: %.2f\n", amount);
        }
        else if(packetId == PACKET_SALE_DATE){
            char date[26];
            memcpy(date,p.data,p.length);
            date[p.length] = '\0';
            printf("Date: %s\n",date);
        }
        else{
            printf("Unknown Packet Id: %d\n",packetId);
        }
    }
}
int main(){
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){
        printf("WSAStartup failed!\n");
        return 1;
    }
    int noofClients;
    do{
        printf("\nEnter no.of clients: ");
        if(scanf("%d",&noofClients)!=1){
            printf("Enter numbers only\n");
            while(getchar()!='\n');
            continue;
        }
        if(noofClients<=0){
            printf("No.of clients be greater than 0\n");
        }
    }while(noofClients<=0);
    for(int i=0;i<noofClients;i++){
        SOCKET client;
        client=socket(AF_INET,SOCK_STREAM,0);
        if(client==INVALID_SOCKET){
            printf("\nSocket creation failed\n");
            continue;
        }      
        sockaddr_in server;
        memset(&server,0,sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port =htons(PORT);
        server.sin_addr.s_addr =inet_addr("127.0.0.1");
        if(connect(client,(sockaddr*)&server,sizeof(server)) == SOCKET_ERROR){
            printf("\nClient %d connection failed\n",i+1);
            closesocket(client);
            continue;
        }
        clients[clientCount]=client;
        clientCount++;
        printf("Client %d connected successfully\n",clientCount);
    }
    if(clientCount==0){
        printf("No clients connected\n");
        WSACleanup();
        return 1;
    }
    int choice;
    while(1){
        printf("\n===================MENU======================\n");
        printf("1. Add Employee\n");
        printf("2. Add Customer\n");
        printf("3. Add Sale\n");
        printf("4. Search\n");
        printf("5. Delete\n");
        printf("6. View All\n");
        printf("7. Exit\n");
        printf("Enter choice: ");
        scanf("%d",&choice);
        if(choice==1){
            int n, clientNo;
            printf("\nEnter no.of Employee: ");
            scanf("%d",&n);
            printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            for(int i=0;i<n;i++){
                printf("\nEmployee %d\n",i+1);
                addEmployee(clients[clientNo-1]);
            }
        }
        else if(choice==2){
            int n, clientNo;
            printf("\nEnter no.of Customer: ");
            scanf("%d",&n);
           printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            for(int i=0;i<n;i++){
                printf("\nCustomer %d\n",i+1);
                addCustomer(clients[clientNo-1]);
            }
        }
        else if(choice==3){
            int n, clientNo;
            printf("\nEnter no.of Sale: ");
            scanf("%d",&n);
            printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            for(int i=0;i<n;i++){
                printf("\nSale %d\n",i+1);
                addSale(clients[clientNo-1]);
            }
        }
        else if(choice==4){
            int clientNo;
            printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            searchEmp(clients[clientNo-1]);
        }
        else if(choice==5){
            int clientNo;
            printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            deleteData(clients[clientNo-1]);
        }
        else if(choice==6){
            int clientNo;
            printf("\nAvailable clients: 1-%d",clientCount);
            printf("Enter Client number: ");
            scanf("%d",&clientNo);
            if(clientNo <1 || clientNo>clientCount){
                printf("Invalid client number\n");
                continue;
            }
            viewAll(clients[clientNo-1]);
        }
        else if(choice==7){
            printf("Exit\n");
            break;
        }
        else{
            printf("Invalid choice\n");
        }
    }
    for(int i=0;i<clientCount;i++){
        closesocket(clients[i]);
    }
    WSACleanup();
    printf("\nAll clients finished\n");
    return 0;
}
