#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define SERVER_SEQ 200

//-------------------------------------------------------------------//
//Estruturas
//-------------------------------------------------------------------//

/*Estrutura TCP Header*/
struct tcp_header{
    short int src;                  //Source port
    short int dst;                  //Destination pot
    int seq;                        //Sequence number
    int ack;                        //Acknowledgment number
    unsigned char tcp_offset: 4;    //Data Offset
    unsigned char tcp_res: 4;       //Reserved 
    short int header_flags;         //SYN, ACK, FIN, etc
    short int win;                  //Window
    int cksum;                      //Checksum
    short int urp;                  //Urgent Pointer
    int opt;                        //Options
};

/*Bit flags dos flags do TCP Header*/
enum{
    SYN = 0x01,
    ACK = 0x02,
    FIN = 0x04,
};

//-------------------------------------------------------------------//
//FUNÇÕES
//-------------------------------------------------------------------//
// Escreve no arquivo server_output.txt as informações de toda a 
// comunicação feita do lado do servidor

void print_tcp_seg(struct tcp_header *tcp_seg){
    FILE *file;

    file = fopen("server_output.txt", "a+");
    
    //Escreve a solicitação de conexao
    printf("Source Port:%hu\n", tcp_seg->src);
    printf("Destination Port: %hu\n", tcp_seg->dst);
    printf("Sequence Number: %hu\n", tcp_seg->seq);
    printf("Acknowledgment Number: %hu\n", tcp_seg->ack);

    if(tcp_seg->header_flags & SYN){
        printf("Header Flags: SYN = 1\n");
    }
    if(tcp_seg->header_flags & ACK){
        printf("Header Flags: ACK = 1\n");
    }
    if(tcp_seg->header_flags & FIN){
        printf("Header Flags: FIN = 1\n");
    }

    printf("Header Flags:%hu\n", tcp_seg->header_flags);
    printf("Window: %hu\n", tcp_seg->win);
    printf("Checksum: 0x%X\n", tcp_seg->cksum);
    printf("Urgent Pointer: %hu\n", tcp_seg->urp);
    printf("Options: %hu\n", tcp_seg->opt);
    printf("-----------\n\n");
    
    //Escreve no arquivo
    fprintf(file, "Source Port:%hu\n", tcp_seg->src);
    fprintf(file, "Destination Port: %hu\n", tcp_seg->dst);
    fprintf(file, "Sequence Number: %hu\n", tcp_seg->seq);
    fprintf(file, "Acknowledgment Number: %hu\n", tcp_seg->ack);

    if(tcp_seg->header_flags & SYN){
        fprintf(file, "Header Flags: SYN = 1\n");
    }
    if(tcp_seg->header_flags & ACK){
        fprintf(file, "Header Flags: ACK = 1\n");
    }
    if(tcp_seg->header_flags & FIN){
        fprintf(file, "Header Flags: FIN = 1\n");
    }

    fprintf(file, "Header Flags:%hu\n", tcp_seg->header_flags);
    fprintf(file, "Window: %hu\n", tcp_seg->win);
    fprintf(file, "Checksum: 0x%X\n", tcp_seg->cksum);
    fprintf(file, "Urgent Pointer: %hu\n", tcp_seg->urp);
    fprintf(file, "Options: %hu\n", tcp_seg->opt);
    fprintf(file, "-----------\n\n");

    fclose(file);
}

//-------------------------------------------------------------------//
// Cria o socket e da bind na porta
// para que ele possa ficar escutando
// retorna o id do socket

int create_server_socket(int port_number){
    int server_socket;
    struct sockaddr_in server;
    
    //Cria o socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(server_socket < 0){
        printf("Socket não pode ser aberto. \n");
        exit(1);
    }   
    printf("Socket criado.\n");
    
    //inicializando a estrutra do socket
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_number);

    //dando bind
    if(bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("Ocorreu um erro ao dar binding.\n");
        exit(1);
    }
    printf("Bind...\n");

    return(server_socket);
}

//-------------------------------------------------------------------//
// Conecta-se a um cliente atraves do socket criado
// Fica escutando a conexao com o cliente
// A função irá aceitar a conexão e tornar o ID do socket para conexao

int connect_client(int server_socket, int  port_number){
    struct sockaddr_in client_addr;
    int size;
    int accept_socket, num_data_recv;
    char buffer[256];
    
    listen(server_socket, 3);
    printf("Escutando na porta %d\n", port_number);

    size = sizeof(struct sockaddr_in);
    int i =0;
    while(i < 256){
        accept_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t*)&size);

        if(accept_socket < 0){
            printf("Erro ao aceitar a conexao tentandando denovo.\n");
            //printf("Erro ao aceitar a conexao.\n");
        //    exit(1);
        }    
        i = i+1;
    }
    printf("Conexao aceita.\n");    

    return accept_socket;
}

//-------------------------------------------------------------------//
// Calcula o checksum

unsigned int compute_cksum(unsigned short int *v_cksum){
    unsigned int i, sum = 0, cksum;
    
    //computa a soma
    for(i = 0; i < 12; i++){
        sum = sum + v_cksum[i];
    }

    //dobra uma vez
    cksum = sum >> 16;
    sum = sum & 0x0000FFFF;
    sum = cksum + sum;

    //dobra mais uma vez
    cksum = sum >> 16;
    sum = sum & 0x0000FFFF;
    sum = cksum + sum;

    //XOR da soma do checksum
    printf("Valor do checksum: 0x%04X\n", (0xFFFF^cksum));
    return(cksum);

}

//-------------------------------------------------------------------//
// O servidor responde ao pedido de conexão, atraves da criação
// de um segmento de conexão TCP

int receive_req(int accept_socket){
    int num_data_recv, num_sent, temp_portnumber;
    char buffer[255];
    struct tcp_header tcp_seg;
    unsigned short int v_cksum[12];

    num_data_recv = read(accept_socket, buffer, 255);
    if(num_data_recv < 0){
        printf("Erro ao receber do socket.\n");
        exit(1);
    }

    memcpy(&tcp_seg, buffer, sizeof(tcp_seg));

    printf("----PEDIDO DE CONEXAO DO CLIENTE----\n");
    print_tcp_seg(&tcp_seg);
    
    //set SYN e ACK para 1
    tcp_seg.header_flags = (SYN | ACK);

    //Atribui um numero de sequencia inicial do servidor ao
    //acknowledgment number igual ao inicial da sequencia do cliente + 1
    tcp_seg.ack = tcp_seg.seq + 1;
    tcp_seg.seq = SERVER_SEQ;

    temp_portnumber = tcp_seg.src;
    tcp_seg.src = tcp_seg.dst;
    tcp_seg.dst = temp_portnumber; //mude o source e destinatio para voltar ao cliente

    //calculo do checksum
    memcpy(v_cksum, &tcp_seg, 24); //copiando 24 bytes
    tcp_seg.cksum = compute_cksum(v_cksum); //calcula o checksum
    printf("Valor do checksum: 0x%04X\n", (0xFFFF^tcp_seg.cksum));
    
    printf("----CONEXAO ACEITA PELO CLIENTE----\n");
    print_tcp_seg(&tcp_seg);

    //envie a mensagem de conexao concedida para o cliente
    memcpy(buffer, &tcp_seg, sizeof tcp_seg); //copie para o buffer
    num_sent = write(accept_socket, buffer, 255); //envie do buffer para o cliente

    if(num_sent < 0){
        printf("Erro ao escrever para o socket. \n");
        exit(1);
    }
    
    return 0;
}

//-------------------------------------------------------------------//
// Recebe o acknowledgement segment do cliente

int receive_ack_seg(int accept_socket){
    int num_data_recv;
    char buffer[255];
    struct tcp_header tcp_ack_seg;

    //recebe acknowledgement TCP do cliente
    num_data_recv = read(accept_socket, buffer, 255);
    if(num_data_recv < 0){
        printf("Erro ao receber dados do socket \n");
        exit(1);
    }
    
    memcpy(&tcp_ack_seg, buffer, sizeof tcp_ack_seg);
    printf("----RECEBENDO ACKNOWLEDGEMENT SEGMENT----\n");
    print_tcp_seg(&tcp_ack_seg);
    
    return 0;
}

//-------------------------------------------------------------------//
// Recebe pedido de fechamendo de conexao

int receive_close_request(int accept_socket){
    int num_data_recv, num_sent, temp_portnumber;
    char buffer[255];
    struct tcp_header tcp_close;
    unsigned short int v_cksum[12];
    
    //recebendo TCP acknowledgement do cliente
    num_data_recv = read(accept_socket, buffer, 255);
    if(num_data_recv < 0){
        printf("Erro ao receber dados do socket");
        exit(1);
    }

    memcpy(&tcp_close, buffer, sizeof tcp_close);

    printf("----RECEBENDO PEDIDO DE FECHAMENTO DE CONEXAO----\n");
    print_tcp_seg(&tcp_close);
    
    temp_portnumber = tcp_close.src;
    tcp_close.src = tcp_close.dst;
    tcp_close.dst = temp_portnumber; //troca o source e o destination para voltar ao cliente
    
    tcp_close.ack = tcp_close.seq + 1;
    tcp_close.header_flags = ACK;

    memcpy(v_cksum, &tcp_close, 24); //copiando 24 bytes
    tcp_close.cksum = compute_cksum(v_cksum);

    printf("-----TRANSMITINDO PRIMEIRO PEDIDO DE FECHAMENTO----\n");
    memcpy(buffer, &tcp_close, sizeof tcp_close); //copie do segmento para o buffer
    num_sent = write(accept_socket, buffer, 255); //envie do buffer para o cliente
    if(num_sent < 0){
        printf("Erro ao escrever no socket.\n");
        exit(1);
    }

    tcp_close.header_flags = FIN;
    tcp_close.seq = SERVER_SEQ;

    memcpy(v_cksum, &tcp_close, 24); //copiando 24 bytes
    tcp_close.cksum = compute_cksum(v_cksum);
    
    printf("----TRANSMITINDO SEGUNDO PEDIDO DE FECHAMENTO----\n");
    print_tcp_seg(&tcp_close);

    bzero(buffer, 255);
    memcpy(buffer, &tcp_close, sizeof tcp_close); //copie do segmento para o buffer
    num_sent = write(accept_socket, buffer, 255); //envie do buffer para o cliente

    if(num_sent < 0){
        printf("Erro ao escever no socket.\n");
        exit(1);
    }
    
    return 0;
}

//-------------------------------------------------------------------//
// Recebe o ack de fechamento de conexao

int receive_close_ack(int accept_socket){
    int num_data_recv;
    char buffer[255];
    struct tcp_header tcp_close;

    //recebendo tcp acknowledgement do cliente
    num_data_recv = read(accept_socket, buffer, 255);
    if(num_data_recv < 0){
        printf("Erro ao receber dados do socket.\n");
        exit(1);
     }

    memcpy(&tcp_close, buffer, sizeof tcp_close);

    printf("----RECEBIDO FECHAMENTO DE CONEXAO----\n");
    print_tcp_seg(&tcp_close);

    return 0;
}


//-------------------------------------------------------------------//
int main (int argc, char **argv){
    //limpando arquivo de saida
    FILE *file;

    
   // server_socket = create_server_socket(port_number);
   // accept_socket = connect_client(server_socket, port_number);

    int socket_desc , accept_socket, c , read_size, port_number;
    struct sockaddr_in server , client;
     
    file = fopen("server_output.txt", "w");
    fclose(file);

    if (argc < 2){
        printf("Numero de porta %s inválido.\n", argv[0]);
        exit(0);
    }
    
    port_number =atoi(argv[1]); //coloca o numero da porta

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_number);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
    //accept connection from an incoming client
    accept_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (accept_socket < 0)
    {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");

    printf("Parte 1\n");

    //Cuidando da solicitação de conexão
    receive_req(accept_socket);
    
    //cuidando do Acknowledgement segment do cliente
    receive_ack_seg(accept_socket);

    printf("Parte 2\n");
    
    //Cuidando do pedido de fechamento de conexao do cliente
    
    receive_close_request(accept_socket);
    receive_close_ack(accept_socket);

    return 0;
}
//-------------------------------------------------------------------//
