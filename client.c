#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CLIENT_SEQ 100

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
// Escreve no arquivo client_output.txt as informações de toda a 
// comunicação feita do lado do client

void print_tcp_seg(struct tcp_header *tcp_seg){
    FILE *file;

    file = fopen("client_output.txt", "a+");
    
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
int connect_server(int port_number){
    struct sockaddr_in server_addr;
    int sock, try_connect;

    //cria o socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock < 0){
        printf("Erro ao criar o socket.\n");
        exit(1);    
    }
    printf("1\n");
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //IP do server, neste caso localhost
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);

    //Conectando ao server
    int i = 0;
//    while (i < 256){
        try_connect = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

        if(try_connect < 0){
            printf("Erro ao tentar conectar. \n");
            perror("");
           // exit(1);
        }
    //    i = i+1;
  //  }
    
    return sock;
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
// Envia o pedido de conexao
// I. Atribui 0 ao acknowledgment do client
// II. Muda o bit SYN para 1
// III. Calcula a soma de verificação do segmento de TCP de 16 bits
//      e preenche o campor do checksum

int send_request(uint32_t source_port, int port_number, int sock){
    int num_sent;
    struct tcp_header tcp_seg;
    unsigned short int v_cksum[12];
    char buffer[255];

    //Adicionando na estrutura TCP de requisicao
    tcp_seg.src = source_port; //setando a source port
    tcp_seg.dst = port_number; //setando a destination port
    tcp_seg.seq = CLIENT_SEQ;  //atribuindo o valor inicial da sequencia do cliente
    tcp_seg.ack = 0;           //colocando 0 no acknowledgement number
    tcp_seg.header_flags = SYN;//setando o SYN para 1
    tcp_seg.win = 0;
    tcp_seg.cksum = 0;
    tcp_seg.urp = 0;
    tcp_seg.opt = 0;

    memcpy(v_cksum, &tcp_seg, 24); //copiando 24 bytes
    tcp_seg.cksum = compute_cksum(v_cksum); //calculando o checksum

    printf("----TRANSMITINDO PEDIDO DE CONEXAO----\n");
    print_tcp_seg(&tcp_seg);

    //envia a estrutura do TCP para o servidor
    memcpy(buffer, &tcp_seg, sizeof(tcp_seg)); //copia a estrutura para o buffer
    num_sent = write(sock, buffer, 255); //escreve no socket
    
    if(num_sent < 0){
        printf("Erro ao escrever no socket.\n");
        exit(1);    
    }
    
    return 0;
}

//-------------------------------------------------------------------//
int send_ack(int sock){
    int num_recv, num_sent, temp_portnumber;
    char buffer[255];
    unsigned short int v_cksum[12];
    struct tcp_header tcp_seg;

    //lendo o segmento de conexao
    num_recv = read(sock, buffer, 255); //lendo do socket
    if(num_recv < 0){
        printf("Erro ao ler do socket. \n");
        exit(1);
    }

    memcpy(&tcp_seg, buffer, sizeof tcp_seg); //copie do buffer para o segmento

    printf("----CONEXAO ESTABELECIDA----\n");
    print_tcp_seg(&tcp_seg);

    //troca o numero das portas
    temp_portnumber = tcp_seg.src;
    tcp_seg.src = tcp_seg.dst;
    tcp_seg.dst = temp_portnumber;

    //Atribui um numero de sequencia inicial do servidor ao
    //acknowledgment number igual ao inicial da sequencia do cliente + 1
    tcp_seg.ack = tcp_seg.seq + 1;

    //Setando o bit ACK para 1
    tcp_seg.header_flags = ACK;

    //Calcula o 16-bit do checksum do TCP inteiro e 
    //popula o campo de checksum
    memcpy(v_cksum, &tcp_seg, 24);
    tcp_seg.cksum = compute_cksum(v_cksum);

    //responde o servidor com acknowledgent
    bzero(buffer, 255);
    memcpy(buffer, &tcp_seg, sizeof tcp_seg);

    printf("----TRANSMITINDO ACK----\n");
    print_tcp_seg(&tcp_seg);

    num_sent = write(sock, buffer, 255); //responde para o servidor
    if(num_sent < 0){
        printf("Erro ao escrever no socket");
        exit(1);
    }
    
    return 0;    
}

//-------------------------------------------------------------------//
// Envia o pedido de fechamento de conexao

int send_close_request(uint32_t source_port, int port_number, int sock){
    int num_sent, num_recv;
    struct tcp_header close_req;
    struct tcp_header close_ack;
    char buffer[255];
    unsigned short int cksum[12];

    close_req.src = source_port; //setando source port
    close_req.dst = port_number; //setando destination pot
    close_req.seq = CLIENT_SEQ;  //atribuindo numero inicial da sequencia
    close_req.ack = 0;  //atribui zero ao acknowledgement
    close_req.header_flags = FIN; //setando o FIN bit para 1
    close_req.win = 0;
    close_req.cksum = 0;
    close_req.urp = 0;
    close_req.opt = 0;

    memcpy(cksum, &close_req, 24);
    close_req.cksum = compute_cksum(cksum);
    
    printf("----TRANSMITINDO PEDIDO DE FECHAMENTO----\n");
    print_tcp_seg(&close_req);
    
    memcpy(buffer, &close_req, sizeof close_req);
    num_sent = write(sock, buffer, 255);
    if(num_sent < 0){
        printf("Erro ao escrever no socket.\n");
        return 1;
    }

    //lendo para o fechamento de conexao
    bzero(buffer, 255);
    num_recv = read(sock, buffer, 255); //lendo do socket
    if(num_recv < 0){
        printf("Erro ao ler do socket.\n");
        exit(1);
    }

    memcpy(&close_ack, buffer, sizeof close_ack); //copia do buffer para o segmento
    
    printf("----PRIMEIRO ACK DE FECHAMENTO DO SERVIDOR----\n");
    print_tcp_seg(&close_ack);

    //lendo o segundo pedido de fechamento
    bzero(buffer, 255);
    num_recv = read(sock, buffer, 255); //lendo do socket
    if(num_recv < 0){
        printf("Erro ao ler do socket.\n");
        exit(1);
    }

    memcpy(&close_ack, buffer, sizeof close_ack); //copia do buffer para o segmento
    
    printf("----SEGUNDO ACK DE FECHAMENTO DO SERVIDOR----\n");
    print_tcp_seg(&close_ack);

    return 0;
}

//-------------------------------------------------------------------//
int send_final_close_ack(uint32_t source_port, int port_number, int sock){
    int num_sent;
    struct tcp_header close_req;
    char buffer[255];
    unsigned short int cksum[12];

    close_req.src = source_port; //setando source port
    close_req.dst = port_number; //setando destination pot
    close_req.seq = CLIENT_SEQ;  //atribuindo numero inicial da sequencia
    close_req.ack = 0;  //atribui zero ao acknowledgement
    close_req.header_flags = FIN; //setando o FIN bit para 1
    close_req.win = 0;
    close_req.cksum = 0;
    close_req.urp = 0;
    close_req.opt = 0;

    memcpy(cksum, &close_req, 24);
    close_req.cksum = compute_cksum(cksum);

    printf("----FECHANDO CONEXAO COM O SERVIDOR----\n");
    print_tcp_seg(&close_req);

    num_sent = write(sock, buffer, 255);
    if(num_sent < 0){
        printf("Erro ao escrever no socket.\n");
        return 1;
    }

    return 0;
}

//-------------------------------------------------------------------//
int main(int argc, char **argv){
    int port_number, sock;
    socklen_t local_addr_len;
    int num_attempts = 0;
    struct sockaddr_in local_addr;
    struct hostnet *server;
    struct tcp_header tcp_seg;

    //limpando arquivo de saida
    FILE *file;

    file = fopen("client_output.txt", "w");
    fclose(file);

    //checa se o usuario passou o numero da porta
    if (argc < 2){
        printf("Numero de porta %s inválido.\n", argv[0]);
        exit(0);
    }

    port_number = atoi(argv[1]); //coloca o numero da porta
    sock = connect_server(port_number); //conecta no server

    //obtem a porta de origem
    local_addr_len = sizeof(local_addr);
    getsockname(sock, (struct sockaddr*) &local_addr, & local_addr_len);

    printf("PARTE 1\n");

    //enviando o pedido de conexao
    send_request(htons(local_addr.sin_port), port_number, sock);

    //recebe a resposta e manda o acknowledgement
    send_ack(sock);

    printf("PARTE 2\n");
    
    //fechando a conexao tcp
    send_close_request(htons(local_addr.sin_port), port_number, sock);
    send_final_close_ack((local_addr.sin_port), port_number, sock);

    return 0;
}
//-------------------------------------------------------------------//
