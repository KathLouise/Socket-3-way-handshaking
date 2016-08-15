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
    printf("----RECEBENDO ACK----\n");
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
    int socket_server, accept_socket, size, port_number;
    struct sockaddr_in server , client;
    
    //limpando arquivo de saida
    FILE *file;  
     
    file = fopen("server_output.txt", "w");
    fclose(file);

    if (argc < 2){
        printf("Numero de porta %s inválido.\n", argv[0]);
        exit(0);
    }
    
    port_number =atoi(argv[1]); //coloca o numero da porta

    //Cria o socket
    socket_server = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_server == -1)
    {
        printf("Não foi possivel criar o sockt.\n");
    }
    puts("Socket criado.");
     
    //Preparando a estrutura
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_number);
     
    //Preparando o Bind
    if(bind(socket_server, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    puts("Bind feito com sucesso.\n");
     
    //Ouvindo
    listen(socket_server , 3);
     
    puts("Esperando pedido de conexao...");
    size = sizeof(struct sockaddr_in);
     
    //aceitando pedido de conexao do cliente
    accept_socket = accept(socket_server, (struct sockaddr *)&client, (socklen_t*)&size);
    if (accept_socket < 0)
    {
        perror("Erro ao aceitar pedido de conexao.");
        return 1;
    }
    puts("Conexao aceita.");

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
