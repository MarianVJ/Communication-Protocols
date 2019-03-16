#ifndef LIB
#define LIB

#define SOH 0x01
#define SINIT 'S'
#define FHEADER 'F'
#define DATA 'D'
#define MEOF 'Z'
#define MEOT 'B'
#define ACK 'Y'
#define NACK 'N'
#define ERROR 'E'

#define MAXL 250
#define TIME 5
#define MARK 0x0D
typedef struct {
    int len;
    char payload[1400];
} msg;

//Structura auxiliara pentru a manevra mai usor headerele din payload
typedef struct{	
	char soh;
	char len;
	char seq;
	char type;
	//TO CHECK
	char data[MAXL];
	unsigned short check;
	char mark;

}frame;


//Structura ajutatoare pt a folosit la creearea pachetului/pachetelor
//de SEND-INIT
typedef struct{
	unsigned char maxl;
	char time;
	char npad;
	char padc;
	char eol;
	char qctl;
	char qbin;
	char chkt;
	char rept;
	char capa;
	char r;
	
} init_frame;

	//Functie care initializeaza zona de data corespunzatoare unui pachet de
	//tipul 'S' ( Send-Init) la valorile corespunzatoare conform cerintei
	void initializeSendInitData(init_frame *ip){
		(*ip).maxl = MAXL;
		(*ip).time = TIME;
		(*ip).npad = 0x00;
		(*ip).padc = 0x00;
		(*ip).eol = 0x0D;
		(*ip).qctl = 0x00;
		(*ip).qbin = 0x00;
		(*ip).chkt = 0x00;
		(*ip).rept = 0x00;
		(*ip).capa = 0x00;
		(*ip).r = 0x00;
	}
	
void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

	
	//Aceasta	functie primeste ca parametru t - pachetul ce va fi transmis
	//seq - numarul de secventa corespunzator (seq)
	//type - tipul pachetului de tranmis
	//lend - lungimea mesajului ce fi tranmiss
	//data un pointer catre mesajul ce va fi transmis
	//Aceasta functie initializeaza atat pachete cu campul data vid cat
	//(data primit ca parametru va fi NULL)
	//si pachete care au campul data nevid (campul data este diferit de NULL)
	void createPacket(msg* t,char * seq , char type,unsigned char lend,char * data){
		frame ft;
		//Setam toti bitii din frame la 0 
		memset(&ft, 0, sizeof(frame));
		
		//Actualiza
		ft.soh = SOH;
		//Setam numarul de secventa
		ft.seq = *seq;
		//Actualizam corespunzatotr nr de secventa pentru urmatorul mesaj
		*seq = (*seq + 1) % 64;
		ft.type = type;
		
		
		//Setam toti bitii din payload la 0
		 memset((*t).payload,0,sizeof((*t).payload));
		 //Copiem primele 4 campuri din frame in payload-ul care va fi
		 //ulterior transmis receptorului
		 memcpy((*t).payload, &ft,4);
		 
		 
		if(data != NULL){
		  	 //Copiem in payload pe pozitia corespunzatoare ( dupa header)
		  	 //datele de transmis si actualizam  campul len din payload
			 memcpy((*t).payload + 4, data, lend);
			 (*t).payload[1] = 2 + lend + 3;
		}
		else{
			//In cazul in care campul data este vid 
			//dupa campul len mai urmeaza doar 2 campuri + crc + 
			//mark (End of Block Marker)
			(*t).payload[1] = 2 + 3;		
		}
		
		//poz_crc este pozitia crc-ului in mesajul curent ( si totodata
		//reprezinta si numarul octetilor scrisi inaintea sa(data + header)
		int poz_crc = 2 + (unsigned char) (*t).payload[1] - 3;
		
		//Calculam crc si il punem pe pozitia corespunzatoare in payload
		unsigned short crc ;
		crc = crc16_ccitt((*t).payload, poz_crc);
	    memcpy((*t).payload + poz_crc,&crc,2);
	    
	    //Am actualizat si mark (End of Block Marker)
		(*t).payload[poz_crc+2] = 0x0D; 
		(*t).len = poz_crc + 3;
	} 
					      
					     


#endif

