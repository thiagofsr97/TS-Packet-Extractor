/*
 * Lavid.c
 *
 * Arquivo principal do projeto.
 *
 *  Criado em: 20 de Junho de 2016
 *  Autor: Thiago Filipe Soares da Rocha - 11502567
 *  Modificado por: (Nenhum)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE *stream; /* Ponteiro para tipo FILE correspondente ao arquivo de vídeo */
FILE *stream2; /* Ponteiro para tipo FILE correspondente ao arquivo de texto*/
FILE *stream3; /* Ponteiro para tipo FILE correspondente ao arquivo de texto */
unsigned char bytes[188];          /* Array de Bytes que guardará os pacotes da TS. */
typedef enum {PAT,PMT} tTableType; /* Indicador de que se quer acesso ao PAT ou PMT*/
unsigned char *prog;               /* Ponteiro para os laços for*/
int *array_prog_map_Pid;           /* Array que guardará todos os Programs Map PIDs */
int paclimit;                      /* Variável que indicará o tamanho dos arrays nas PAT/PMT Table*/
int limitcount;                    /* Contador do laço que indica quantos elementos correspondentes
                                      ao Program_Map_PID existem */



/**
 *  TransStreamPack (Structure): Estrutura contendo as variáveis correspondentes
 *  aos dados da Transport Stream Pack.
 */

typedef struct {
    int sync_byte;
	int transport_error_indicator;
	int payload_unit_start_indicator;
	int transport_priority;
	int PID;
	int transport_scrambling_control;
	int adaptation_field_control;
	int continuity_counter;
}TransStreamPack;

/**
 *  PatInfo(Structure): Estrutura contendo as variáveis correspondentes
 *  aos dados da PAT Table.
 */

typedef struct{
    int n_bytes_pointer_field;
    int table_id;
	int section_syntax_indicator;
	int bitszero;
	int reserved;
	int section_length;
	int transport_stream_id;
	int reserved1;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int program_numbertemp;
	int *program_number;
	int *program_map_PID;
	unsigned int CRC_32;
}PatInfo;

/**
 *  PmtInfo(Structure): Estrutura contendo as variáveis correspondentes
 *  aos dados da PMT Table.
 */
typedef struct{
    int n_bytes_pointer_field;
    int table_id ;
	int section_syntax_indicator ;
	int bitszero;
	int section_length;
	int program_number;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int PCR_PID;
	int reserved3;
	int program_info_length;
    int *stream_type;
    int *elementary_PID;
	int *ES_info_length;
	unsigned int CRC_32;
}PmtInfo;

/**
 *  UniDeslc: Recebe dois bytes de um array e une ambos num único int.
 *  Parametros:
 *  	bytes (entrada): primeira string binaria;
 *      bytes2 (entrada): segunda string binaria.
 *  Retorno:
 *  	Retorna o int resultante da união.
 *
 */

int UniDeslc ( int bytes,int bytes2){
    int resultado = 0;;
    bytes <<= 8;
    resultado = bytes ^ bytes2;

    return resultado;

}

 /**
 *  UniDeslc4: Recebe quatro bytes de um array e une os quatro num único int.
 *  Parametros:
 *  	bytes (entrada): primeira string binaria;
 *      bytes2 (entrada): segunda string binaria.
 *      bytes3 (entrada): terceira string binaria;
 *      bytes4 (entrada): quarta string binaria.
 *
 *  Retorno:
 *  	Retorna o int resultante da união.
 *
 */

int UniDeslc4 (int bytes,int bytes2,int bytes3, int bytes4){
    unsigned int resultado = 0;
    bytes <<= 24;
    bytes2 <<= 16;
    bytes3 <<= 8;
    resultado = bytes ^ bytes2 ^ bytes3 ^ bytes4;

    return resultado;

}

/**
 *  TransportStream: Atribui os bytes do pacote de 188 bytes a suas informações correspondentes
 *  da Transport Stream Packet.
 *  Parametros:
 *  	tabletype (entrada): variável indicando o que está se procurando (PAT ou PMT).
 *  Retorno:
 *  	Retorna um indicador de que há mais um pacote PAT ou PMT ou -1 em caso de erro.
 *
 */

int TransportStream(tTableType tabletype){
        TransStreamPack pkt;
        int indicnextTable,i;

        pkt.sync_byte = bytes[0] & 0xFF; /*Isolar os 8 bits*/
		pkt.transport_error_indicator = ((bytes[1] & 0xFF) >> 7) & 0x01; /*Isolar 1 bit deslocando 7 casas e zerando o restante*/
		pkt.payload_unit_start_indicator = ((bytes[1] & 0xFF) >> 6) & 0x01; /*Isolar 1 bit deslocando 6 casas e zerando o restante*/
		pkt.transport_priority = ((bytes[1] & 0xFF) >> 5) & 0x01; /*Isolar 1 bit deslocando 5 casas e zerando o restante*/
        pkt.PID = UniDeslc(((bytes[1] & 0xFF) & 0x1f),(bytes[2] & 0xFF)) & 0xFFFF ; /*Isolar 13 bits unindo os bits correspondentes em 2 bytes*/
		pkt.transport_scrambling_control = ((bytes[3] & 0xFF) >> 6) & 0x03; /*Isolar 2 bits deslocando 6 casas e zerando o restante*/
		pkt.adaptation_field_control = ((bytes[3] & 0xFF) >> 4) & 0x03; /*Isolar 2 bits deslocando 4 casas e zerando o restante*/
		pkt.continuity_counter = (bytes[3] & 0xFF) & 0xF; /*Isolar 4 bits sem deslocar casas e zerando o restante*/

		/*Se todas as condições abaixo forem verdadeiras, significa dizer que:
		 1 - PID igual a 0, então temos um PAT Table no pacote em questão, caso seja igual a um dos elementos do
             array_prog_map_PID, então temos um PMT Table;
		 2 - payload_unit_star_indicator igual a 1, então o pacote pode carregar o primeiro byte da PSI Section
             logo após o pointer_field, que também estará contido no pacote;
		 3 - pkt.adaptation_field_control igual a 1, então está presente apenas a PAT Table no pacote.
		*/
		if(tabletype == PAT){
            if (pkt.PID == 0 && pkt.payload_unit_start_indicator == 1 && pkt.adaptation_field_control == 1){
                indicnextTable = PatTable();
                return indicnextTable;
            }

		}
		else if (tabletype == PMT){
		    for(i = 0; i<limitcount;i++){
                if(pkt.PID == array_prog_map_Pid[i] && pkt.payload_unit_start_indicator == 1 && pkt.adaptation_field_control == 1)
                    indicnextTable = PmtTable();
            }
            return indicnextTable;
        }



return -1;

}

/**
 *  PatTable: Atribui os bytes do pacote de 188 bytes a suas informações correspondentes da Pat Table.
 *  Parametros:
 *  	(nenhum).
 *  Retorno:
 *  	Retorna um indicador de que há mais um pacote PAT ou não.
 *
 */

int PatTable(){
    PatInfo patinfo;

    patinfo.n_bytes_pointer_field = (bytes[4] & 0xFF); /* Indica onde o payload inicia*/
    patinfo.table_id = bytes[5 + patinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    patinfo.section_syntax_indicator = ((bytes[6 + patinfo.n_bytes_pointer_field] & 0xFF) >> 7) & 0x01; /* Isolar 1 bit deslocando 7 casas*/
    patinfo.bitszero = ((bytes[6 + patinfo.n_bytes_pointer_field] & 0xFF) >> 6) & 0x01; /* Isolar 1 bit deslocando 6 casas e zerando o restante*/
    patinfo.reserved = ((bytes[6 + patinfo.n_bytes_pointer_field] & 0xFF) >> 4) & 0x03; /* Isolar 2 bits deslocando 4 casas e zerando o restante*/
    patinfo.section_length = UniDeslc(((bytes[6 + patinfo.n_bytes_pointer_field] & 0xFF) & 0xF),
                                      (bytes[7 + patinfo.n_bytes_pointer_field] & 0xFF))  & 0xFFF; /* Isolar 12 bits dos 2 bytes*/
    patinfo.transport_stream_id = UniDeslc((bytes[8 + patinfo.n_bytes_pointer_field] & 0xFF),
                                           (bytes[9 + patinfo.n_bytes_pointer_field] & 0xFF))& 0xFFFF; /* Isolar 16 bits dos 2 bytes*/
    patinfo.reserved1 = ((bytes[10 + patinfo.n_bytes_pointer_field] & 0xFF) >> 6) & 0x03; /* Isolar 2 bits deslocando 4 casas e zerando o restante*/
    patinfo.version_number = ((bytes[10 + patinfo.n_bytes_pointer_field] & 0xFF) >> 1) & 0x1F; /* Isolar 5 bit deslocando 1 casa e zerando os 3 primeiros*/
    patinfo.current_next_indicator = (bytes[10 + patinfo.n_bytes_pointer_field] & 0xFF) & 0x01; /*Isolar o ultimo bit do byt, não necessita deslocamento*/
    patinfo.section_number = bytes[11 + patinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    patinfo.last_section_number = bytes[12 + patinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    patinfo.CRC_32 = UniDeslc4((bytes[4 + patinfo.n_bytes_pointer_field + patinfo.section_length ] & 0xFF),
                               (bytes[5+ patinfo.n_bytes_pointer_field + patinfo.section_length] & 0xFF),
                               (bytes[6 + patinfo.n_bytes_pointer_field + patinfo.section_length ] & 0xFF),
                               (bytes[7 + patinfo.n_bytes_pointer_field + patinfo.section_length] & 0xFF)); /* Isolar 32 bits dos 4 bytes*/


    paclimit = (patinfo.section_length - 9)/4; /*Variável que ditará o tamanho dos arrays correspondentes ao program
                                                 _number e program_map_PID,para saber quantos elementos desse tipo
                                                 podemos ter no máximo. A divisão por 4 deve-se ao fato de que são
                                                 necessários no mínimo 4 bytes para armazenar essas informações   */

    /*Alocação do tamanho dos arrays por meio do paclimit */
    patinfo.program_number =  malloc(paclimit * sizeof(patinfo.program_number));
    patinfo.program_map_PID = malloc(paclimit * sizeof(patinfo.program_map_PID));
    array_prog_map_Pid = malloc(paclimit * sizeof(array_prog_map_Pid));

    /* O laço a seguir será percorrido até antes do array prog alcançar os bytes correspondentes
    ao CRC_32 (4 bytes) e também 5 bytes antes disso, uma vez que esses bytes foram usados (bytes[8]
    a bytes[12]. Seu incremento será de 4, pois é o mínimo necessário para se armazenar as informações
    desejadas.                                            .                                         */
    for (prog = bytes + 13 + patinfo.n_bytes_pointer_field,limitcount=0;prog < bytes + 13 + patinfo.section_length - 9; prog += 4){

          patinfo.program_numbertemp = UniDeslc((prog[0] & 0xFF),(prog[1] & 0xFF));
          if (!(patinfo.program_numbertemp == 0)){
                patinfo.program_number[limitcount] = patinfo.program_numbertemp;
                patinfo.program_map_PID[limitcount] = UniDeslc(((prog[2] & 0xFF) & 0x1f),(prog[3] & 0xFF));
                array_prog_map_Pid[limitcount] = patinfo.program_map_PID[limitcount];
                limitcount++;
          }
    PatInfo *pointer_pi = &patinfo;
    PrintPatInfo(pointer_pi);

    }
    return patinfo.current_next_indicator;
}

/**
 *  PmtTable: Atribui os bytes do pacote de 188 bytes a suas informações correspondentes da PMT Table.
 *  Parametros:
 *  	(nenhum).
 *  Retorno:
 *  	Retorna um indicador de que há mais um pacote PMT ou não.
 *
 */

int PmtTable(){
    PmtInfo pmtinfo;

    pmtinfo.n_bytes_pointer_field = (bytes[4] & 0xFF); /* Indica onde o payload inicia*/
    pmtinfo.table_id = bytes[5 + pmtinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    pmtinfo.section_syntax_indicator = ((bytes[6 + pmtinfo.n_bytes_pointer_field] & 0xFF) >> 7) & 0x01; /* Isolar 1 bit deslocando 7 casas*/
    pmtinfo.bitszero = ((bytes[6 + pmtinfo.n_bytes_pointer_field] & 0xFF) >> 6) & 0x01; /* Isolar 1 bit deslocando 6 casas e zerando o restante*/
    pmtinfo.section_length = UniDeslc(((bytes[6 + pmtinfo.n_bytes_pointer_field] & 0xFF) & 0xF),(bytes[7 + pmtinfo.n_bytes_pointer_field] & 0xFF))  & 0xFFF; /* Isolar 12 bits dos 2 bytes*/
    pmtinfo.program_number = UniDeslc((bytes[8 + pmtinfo.n_bytes_pointer_field] & 0xFF), (bytes[9 + pmtinfo.n_bytes_pointer_field ] & 0xFF)) & 0xFFFF;
    pmtinfo.version_number = ((bytes[10 + pmtinfo.n_bytes_pointer_field] & 0xFF) >> 1) & 0x1F; /* Isolar 5 bits deslocando 1 casas e zerando o restante*/
    pmtinfo.current_next_indicator = ((bytes[10 + pmtinfo.n_bytes_pointer_field] & 0xFF) & 0x1); /* Isolar 1 bit sem deslocar casas e zerando o restante*/
    pmtinfo.section_number = bytes[11 + pmtinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    pmtinfo.last_section_number = bytes[12 + pmtinfo.n_bytes_pointer_field] & 0xFF; /* Isolar os 8 bits*/
    pmtinfo.PCR_PID = UniDeslc(((bytes[13 + pmtinfo.n_bytes_pointer_field] & 0xFF) & 0x1F), (bytes[14 + pmtinfo.n_bytes_pointer_field] & 0xFF)) & 0xFFFF; /* Isolar 13 bits dos 2 bytes*/
    pmtinfo.program_info_length = UniDeslc(((bytes[15 + pmtinfo.n_bytes_pointer_field] & 0xFF) & 0xF), (bytes[16 + pmtinfo.n_bytes_pointer_field] & 0xFF)) & 0xFFFF;



    paclimit = (pmtinfo.section_length - 13 - pmtinfo.program_info_length)/5; /*Variável que ditará o tamanho dos arrays correspondentes
                                                                                   as informações dos streams.  */
    /*Alocação do tamanho dos arrays por meio do paclimit */

    pmtinfo.stream_type =  malloc(paclimit * sizeof(pmtinfo.stream_type));
    pmtinfo.elementary_PID = malloc(paclimit * sizeof(pmtinfo.elementary_PID));
    pmtinfo.ES_info_length = malloc(paclimit * sizeof(pmtinfo.ES_info_length));


    /* O laço a seguir será percorrido até antes do array prog alcançar os bytes correspondentes
    ao CRC_32 (4 bytes) e também 9 bytes antes disso, uma vez que esses bytes foram usados (bytes[8]
    a bytes[16]. Seu incremento será de 5, pois é o mínimo necessário para se armazenar as informações
    desejadas.                                            .                                         */
    for (prog = bytes + 17 + pmtinfo.program_info_length + pmtinfo.n_bytes_pointer_field,limitcount=0;prog < bytes + 17 +
         pmtinfo.section_length - 13 - pmtinfo.program_info_length ; prog += 5){

          pmtinfo.stream_type[limitcount] = prog[0] & 0xFF;
          pmtinfo.elementary_PID[limitcount] = UniDeslc(((prog[1] & 0xFF) & 0x1F),(prog[2] & 0xFF)) & 0xFFFF;
          pmtinfo.ES_info_length[limitcount] = UniDeslc(((prog[3] & 0xFF) & 0xF),(prog[4] & 0xFF)) & 0xFFFF;
          limitcount++;
    }
     pmtinfo.CRC_32 = UniDeslc4((bytes[4 + pmtinfo.n_bytes_pointer_field + pmtinfo.section_length] & 0xFF),
                               (bytes[5 + pmtinfo.n_bytes_pointer_field + pmtinfo.section_length] & 0xFF),
                               (bytes[6 + pmtinfo.n_bytes_pointer_field + pmtinfo.section_length] & 0xFF),
                               (bytes[7 + pmtinfo.n_bytes_pointer_field  + pmtinfo.section_length] & 0xFF));

    PmtInfo *pointer_pm = &pmtinfo;
    PrintPmtInfo(pointer_pm);

    return pmtinfo.current_next_indicator;
}


void PrintPatInfo (PatInfo *patinfo){
    int i;
    stream2 = fopen("PAT Table.txt","a");
    if(stream2==NULL){
       printf("Erro ao abrir arquivo.\n");
       exit(1);
   }


fprintf(stream2,"Printing PMT Table(s)...\n\n");

fprintf(stream2,"Pointer_Field: %d byte(s)\n"
       "Table_ID: %d\n"
       "Section_Syntax_Indicator: %d\n"
       "Section_Length: %d\n"
       "Transport_Stream_Id: %d\n"
       "Version_Number: %d\n"
       "Current_Next_Indicator: %d\n"
       "Section_Number: %d\n"
       "Last_Section_Number: %d\n",patinfo->n_bytes_pointer_field,patinfo->table_id,
       patinfo->section_syntax_indicator,patinfo->section_length,patinfo->transport_stream_id,
       patinfo->version_number,patinfo->current_next_indicator,patinfo->section_number,
       patinfo->last_section_number);

for(i=0;i<limitcount;i++){
    fprintf(stream2,"Program_Number: %d\n"
           "Program_Map_PID: 0x%x\n\n",patinfo->program_number[i],
           patinfo->program_map_PID[i]);
}
fprintf(stream2,"CRC_32: %u\n\n",patinfo->CRC_32);

fclose(stream2);
return;
}

void PrintPmtInfo (PmtInfo *pmtinfo){
    int i;
    stream3 = fopen ("PMT Table.txt","a");
    if(stream3 == NULL){
        printf("Erro ao abrir arquivo.\n");
        exit(2);
    }

fprintf(stream3,"Printing PMT Table(s)...\n\n");

fprintf(stream3,"Pointer_Field: %d byte(s)\n"
       "Table_ID: %d\n"
       "Section_Syntax_Indicator: %d\n"
       "Section_Length: %d\n"
       "Program_Number: %d\n"
       "Version_Number: %d\n"
       "Current_Next_Indicator: %d\n"
       "Section_Number: %d\n"
       "Last_Section_Number: %d\n"
       "PCR_PID: 0x%x\n"
       "Program_Info_Length: %d\n\n",
       pmtinfo->n_bytes_pointer_field,pmtinfo->table_id,
       pmtinfo->section_syntax_indicator,pmtinfo->section_length,pmtinfo->program_number,
       pmtinfo->version_number,pmtinfo->current_next_indicator,pmtinfo->section_number,
       pmtinfo->last_section_number,pmtinfo->PCR_PID,pmtinfo->program_info_length);

 for(i=0;i<limitcount;i++){
    fprintf(stream3,"Stream_Type:: 0x%x\n"
           "Elementary_PID: 0x%x\n"
           "ES_Info_Length: %d\n\n",
           pmtinfo->stream_type[i],pmtinfo->elementary_PID[i],
           pmtinfo->ES_info_length[i]);
}
fprintf(stream3,"CRC_32: %u\n\n",pmtinfo->CRC_32);

fclose(stream3);
return;
}


int main (void){
    tTableType tabletype;

/* Criando "PAT Table.txt" para salvar os dados extraídos da PAT*/
   stream2 = fopen("PAT Table.txt","w");
   if(stream2==NULL){
       printf("Erro ao abrir arquivo.\n");
       return -1;
   }
   fclose(stream2);

/* Criando "PMT Table.txt" para salvar os dados extraídos da PMT*/

   stream3 = fopen("PMT Table.txt","w");
   if(stream3 = NULL){
        printf("Erro ao abrir arquivo.\n");
        return -1;
   }
   fclose(stream3);

/*Abrindo arquivo de vídeo para leitura em modo binario*/

   stream = fopen ("video.ts","rb");
   if(stream==NULL){
       printf("Erro ao abrir arquivo.\n");
       return -1;
   }

   /* Lendo o pacote de 188 bytes para obtenção dos dados do PAT Table*/
   while(1){
        fread( bytes, sizeof(bytes), 1, stream);
        /*Passando o pacote de 188 bytes para obtenção dos dados
          referentes ao Transport Stream Packet e verificando se
          haverá mais de um PAT Table presente nos pacotes. */
        if(TransportStream(PAT) == 1 || feof(stream))
           break;
   }
   rewind (stream);
   if(feof(stream)){
       return -1;
    }

   /* Lendo o pacote de 188 bytes para obtenção dos dados do PMT Table*/
   while(1){
        fread( bytes, sizeof(bytes), 1, stream);
        /*Passando o pacote de 188 bytes para obtenção dos dados
          referentes ao Transport Stream Packet e verificando se
          haverá mais de um PMT Table presente nos pacotes.    */
        if(TransportStream(PMT) == 1 || feof(stream))
           break;

   }

   fclose(stream);

   printf("PAT and PMT Tables extracted sucessfully.\n");



return 0;
}
