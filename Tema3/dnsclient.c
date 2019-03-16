#include<stdio.h>
#include "lib.h" 
 
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>



char *buffer;
char *first_buf;
int copie;
int contor;
int poz;
FILE *fm;	

//Functia care primeste un String si returneaza un nr corespunzator
//unui tip conform cu specificatiile RFS 1035
int  toType(char *s){
		
		if(strncmp(s,"A",1) == 0){
			return 1;
		}else if (strncmp(s,"NS",2) == 0){
			return 2;	
		}else if (strncmp(s,"CNAME",5) == 0){
			return 5;
		}else if (strncmp(s,"MX",2) == 0){
			return 15;
		}else if(strncmp(s,"SOA",3) == 0){
			return 6;
		}else if(strncmp(s,"TXT",3) == 0){
			return 16;
		}else if(strncmp(s,"PTR",3) == 0){
			return 12;
		}
}
		
//Functia care primeste un numar si returneaza un String cu numele
//sectiunii corespunzatoare
char *fromClass(unsigned short t)
{
	char *tip = calloc(10,1);
	if(t == 1){
		strcpy(tip,"IN");
		return tip;
	}
	else if ( t == 2){
		strcpy(tip,"Unassigned");
		return tip;
	}
	else if (t == 3){
		strcpy(tip,"CH");
		return tip;
	}
	else if(t == 4)
	{
		strcpy(tip,"HS");;
		return tip;
	}
	else if(t == 254){
		strcpy(tip,"NONE");
		return tip;
	}
	else if(t == 255){
		strcpy(tip,"ANY");
		return tip;
	}	 
}
//Functia care returneaza un String corespunzator unui tip 
char* fromType(int t){
	char *tip = calloc(5,1);
	if(t == 1){
		strcpy(tip,"A");
		return tip;
	}
	else if(t == 2){
		strcpy(tip,"NS");
		return tip;
	}
	else if(t == 5){
		strcpy(tip,"CNAME");
		return tip;	
	}else if (t == 15){
		strcpy(tip,"MX");
		return tip;
	}else if(t == 6){
		strcpy(tip,"SOA");
		return tip;
	}
	else if(t == 16){
		strcpy(tip,"TXT");
		return tip;
	}
}


//Citeste adresa de DNS din fisierul de adrese si ignora comentariile
char* CitesteAdresa(FILE *f)
{
	char dns[30];
	memset(dns,0,30);
	char buffer[250];
	int flag;
	size_t l;

	while(1){
		
		memset(buffer,0,250);
		fgets(buffer, 250, f);
		if(buffer == NULL){
				printf("Eroare de citire");
		}
		
		if(buffer[0] != '#'){
			strcpy(dns,buffer);
			break;
		}	
	}
	char *rez = buffer;
	return rez;
}

//Aceasta functie intoarce un string de la o anumita pozitie 
//si testeaza la fiecare pas , daca am intalnit un pointer
char* verificaPointer(int pozz)
{
	char* rezultat = calloc(250,1);

	unsigned short pointer;
	unsigned short j,i = 0;
	int len;
	int flag = 0;
	//Cat timp nu intalnim 0
	while(buffer[pozz] != 0){	
	
		//Verificam daca este pointer 
		if((unsigned char )buffer[pozz] >= 192){
			memcpy(&pointer, buffer + pozz, 2);
			pointer = ntohs(pointer);
			pointer = pointer <<2;
			pointer = pointer >> 2;

			if(flag == 0)
				contor  = contor + 2;
			
			flag = 1;
			pozz = pointer;
		}else{
			
			if(flag == 0){
				contor++;
			}
			len = buffer[pozz]; //Daca afisez direct buffer[poz] , poz ul se modifica
								//asa ca salvezi dimensiunea
			pozz++;
			//Transform din QNAME in name 
			for(j = 0  ; j < len ; j++){
				rezultat[i] = buffer[pozz];
				pozz++;
				i++;
				if(flag == 0){
					contor++;
				}
			}
			rezultat[i] = '.';
			i++;
		}
	}
	return rezultat;	
}

void afiseazaSectiune(int nrsectiune)
{
	dns_rr_t answer;	
	int flag = 0;
	
	//Cat timp avem mai avem mesaje in sectiunea curenta
	while(nrsectiune > 0){

		int i;
		contor = 0;
		//Iau numele de inceput cu ajutorul functiei auxiliare
		char *pointer =  verificaPointer(poz);
		//Updatez pozitia actuala fata de inceputul sirului , cu ajutorull
		//variabilei contor , care a fost modificata in functie
		//Exista doua cazuri , primul short a fost pointer , iar atunci
		//contor este 2 sau , Daca apoi avem un string , contor contine
		//dimensiunea stringului ( si eventual daca mai exista si un pointer
		//dupa)
		poz = poz  + contor;
		
		//Iau answer-ul 
		memcpy(&answer, buffer +  poz,sizeof(dns_rr_t));
		
		//Convertim answerul de la BIG LA LITTLE ENDIAN
		answer.type     = ntohs(answer.type);
		answer.class    = ntohs(answer.class);
		answer.ttl      = ntohl(answer.ttl);
		answer.rdlength = ntohs(answer.rdlength);
			
		//Updatez pozitia ( scad -2 ) deoarece desi structura ocupa 
		//10 octeti , seizoef intoarce 12 din cauza ca se face padding
		//la structuri ca sa fie multiplu de 4
		poz = poz + sizeof(dns_rr_t)-2;

		flag = 0;
		//printf("TYE : %d \n", answer.rdlength);
		
		
		//ANSWER de tip "A"
		if(answer.type == A){
			
			fprintf(fm, "%s\t%d\t%s\t%s\t",pointer,answer.ttl,
										fromClass(answer.class),fromType(answer.type));
			
			//Printam adresa IP
			int pozz = poz;
			for(i = 0 ; i < 4 ; i++){
				fprintf(fm, "%d",(unsigned char) buffer[pozz]);
				pozz++;
				if(i < 3){
					printf(".");
				}
			}
			fprintf(fm, "\n");
		}
		
		//ANSWER de tip "NS" "PTR" "CNAME"
		if(answer.type == NS || answer.type == PTR || answer.type == CNAME){		
			
			fprintf(fm, "%s\t%d\t%s\t%s\t",pointer,answer.ttl,
										fromClass(answer.class),fromType(answer.type));
			//contor trebui reinitializat cu 0 pt a updata corect pozitia
			//relativa fata de inceputul sirului , dupa apelarea functiei 
			//verificaPointer 
			contor = 0;
			fprintf(fm, "%s", verificaPointer(poz));
			fprintf(fm, "\n");
		}	
		//ANSWER de tip "MX"
		if(answer.type == MX){	
			fprintf(fm, "%s\t%d\t%s\t%s\t",pointer,answer.ttl,
										fromClass(answer.class),fromType(answer.type));
			unsigned short preference;
			memcpy(&preference, buffer + poz, 2);
			preference = ntohs(preference);
			fprintf(fm, "%d\t", preference);
			
			fprintf(fm, "%s", verificaPointer(poz+2));
			fprintf(fm, "\n");
		}
		//ANSWER de tip "SOA"
		if(answer.type == SOA){
			fprintf(fm, "%s\t%d\t%s\t%s\t",pointer,answer.ttl,
										fromClass(answer.class),fromType(answer.type));
			int pozz = poz;
			contor = 0;
			fprintf(fm, "%s ", verificaPointer(pozz));
			pozz = pozz+contor;
					
			contor = 0;
			fprintf(fm, "%s ", verificaPointer(pozz));
			pozz = pozz+contor;
			
			unsigned int  serial_number;
			memcpy(&serial_number, buffer + pozz, 4);
			serial_number = ntohl(serial_number);
			pozz = pozz+4;
			
			unsigned int refresh_interval;
			memcpy(&refresh_interval, buffer + pozz, 4);
			refresh_interval = ntohl(refresh_interval);
			pozz = pozz+4;
			
			//Stiu ca in specificatii 
			unsigned int  retry_interval;
			memcpy(&retry_interval, buffer + pozz, 4);
			retry_interval = ntohl(retry_interval);
			pozz = pozz+4;
			
			unsigned int  expire_limit;
			memcpy(&expire_limit, buffer + pozz, 4);
			expire_limit = ntohl(expire_limit);
			pozz = pozz+4;
			
			unsigned int  minimum_ttl;
			memcpy(&minimum_ttl, buffer + pozz, 4);
			minimum_ttl = ntohl(minimum_ttl);
			pozz = pozz+4;
			
			fprintf(fm, "%d %d %d %d %d\n",serial_number,refresh_interval,retry_interval,
									expire_limit, minimum_ttl);
				
		}
		//ANSWER de tip "TXT"
		if(answer.type == TXT){
			fprintf(fm, "%s\t%d\t%s\t%s\t",pointer,answer.ttl,
										fromClass(answer.class),fromType(answer.type));
			int pozz = poz;
			contor = 0;
			fprintf(fm, "%s\n", verificaPointer(pozz));
			pozz = pozz+contor;	
		}
		
		//Lungimea campului variabil rdata este continuta in answer rdlength 
		//si updatam poz 
		poz = poz + answer.rdlength;	
		nrsectiune--;	
	}	
}


int main(int argc, char **argv)
{

	if(argc < 3)
	{
		printf("Nu sunt suficiente argumente\n");	
	}
	
	char qname[100];
	char tip[10];
	char domeniu[100];
	char dns[30];
	char dns_copie[30];
	memset(dns_copie,0, 30);
	
	memset(dns, 0,30);
	memset(domeniu,0,100);
	memset(tip,0,10);
	

	strcpy(domeniu, argv[1]);
	strcpy(tip,argv[2]);
	
	
	//Inversez ordinea , si adaug in-addr.arpa in cazul in care tip
	//este PTR
	if(strncmp(tip,"PTR",3) == 0){	
		memset(qname , 0 ,100);
		char *tok = strtok (domeniu, ".");
		char *p1,*p2,*p3,*p4;
		int cont = 0;
		
		while(tok != NULL){
			if(cont == 0){
				p1 = tok;
			}else if(cont == 1){
				p2 = tok;
			}else if(cont == 2){
				p3 = tok;
			} else{
				p4 = tok;
			}
			cont++;
			strcat(qname , tok);
			tok = strtok(NULL, ".");
		}
		//Inversez ordinea numerelor din ip si adaug "in-addr.arpa"
		strcpy(qname,p4);
		strcat(qname,".");
		strcat(qname,p3);
		strcat(qname,".");
		strcat(qname,p2);
		strcat(qname,".");
		strcat(qname,p1);
		strcat(qname,".in-addr.arpa");
		printf("%s\n",qname);
		memset(domeniu, 0 , 100);
		strcpy(domeniu, qname);	
	}
		
	
	//Construim QNAME-ul	
	memset(qname , 0 ,100);	
	char *tok = strtok (domeniu, ".");
	while(tok != NULL){
		qname[strlen(qname)]= strlen(tok);
		strcat(qname , tok);
		tok = strtok(NULL, ".");
	}
	
	dns_header_t header;
	memset(&header, 0,sizeof(header));
	dns_question_t question;
	memset(&question,0,sizeof(question));
		
	//Completam header-ul
	header.id = 1;	
	header.rd = 1; //recursivitatea setata pt dns 
	header.qdcount = htons(1); // trcem de la big la little endian
	
	//Completam question-ul 
	question.qtype = htons(toType(tip));
	question.qclass = htons(1);
	
	//Aceasta cerere va fi transmisa catre serverul de DNS	
	char cerere[500];
	memset(cerere, 0, 500);
	
	//Construim mesajul corespunzator 
	memcpy(cerere, &header, sizeof(header));
	memcpy(cerere + sizeof(header), qname, strlen(qname)+1);
	memcpy(cerere  + sizeof(header) + strlen(qname)+1, &question, sizeof(question));
	
	//Deschidem fisierul un care se afla ip-urile serverelor de DNS
	FILE *f = fopen("dns_servers.conf","r");
	
	

	int BUFLEN = 512;
	char buf[BUFLEN];
		
	//Socket UDP 
	int fd;
	struct sockaddr_in serv_addr;
	
	int ln_msg,initial_len;
	int flag = 0;
	char *aux;
	
	//Incercam sa primim un raspuns in maxim 3 secunde de la unul din serverele 
	//al caror ip se afla in fisierul dns_servers.conf
	while(1)
	{
		memset(dns, 0 ,30);
		aux  = CitesteAdresa(f);
		strcpy(dns, aux);
		dns[strlen(dns)-1] = 0;
		
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(53);
		inet_aton(dns, &serv_addr.sin_addr);
		
		if ((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0){
			printf("S-a produs o eroare la deschiderea socketului:");
		}
		
		if (connect(fd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
			printf("S-a produs o eroare la conectare: ");
		}

		initial_len = send(fd, &cerere,sizeof(question)+  sizeof(header) + strlen(qname)+1,0);
		
		memset(buf,0,512);
		//Vom astepta maxim 5 secunde pentru un raspuns 
		struct timeval t;
		t.tv_sec = 5;
		t.tv_usec = 0;

		//Setam timeout-ul 
		setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char*) &t, sizeof (struct timeval));
		ln_msg = recv(fd, buf, BUFLEN, 0);
		
		//Verificam daca am primit vreoun mesaj de la serverul curent
		if (ln_msg <= 0) {
			printf(" Am primit timeout la receptie de la serverul dns %s.\n", dns);
			close(fd);
		}
		else{
			printf("Am primit raspuns de la serverul DNS cu ip-ul: %s\n", dns);
			
			//Deschidem fisierul de dns.log si appenduim noul raspuns pt comanda
			//data , 
			fm = fopen("dns.log", "a");	
			fprintf(fm,"\n\n; %s - %s %s\n", dns, argv[1], argv[2]);
			flag = 1;
			break;
		}
	}
	fclose(f);
	//Daca niciunul din serverele din fisierul dns_servers.conf nu este valid 
	//atunci programul ia sfarsit
	if(flag == 0){
		printf("Nu s-a putu face conectarea la niciunul din serverele dns\n");
		return 0;
	}

	memset(&header, 0 , sizeof(header));
	//Copiem headerul din raspunsul primi de la server
	memcpy(&header, buf, sizeof(buf));
	
	f = fopen("message.log", "wt");
	fprintf(f, "Pentru comanda \"%s %s\" am primit codul hexa: \n",argv[1],argv[2]);
	
	//Scriem mesajul in hexa , in acelasi format in care sunt afisate datele
	//hexa in programul WireShark
	int i;
	for(i = 0 ; i < ln_msg ; i++){
		fprintf(f, "%x ",buf[i] & 0xff);
		if((i+1) % 16 == 0){
			fprintf(f,"\n");
		}
	}
	//Inchidem fisierul in care am scris datele in hexa 
	fclose(f);

	//ANCOUNT - speficica numarul de RR din secitunea Answer
	//NSCOUNT - specifica numarul de RR din sectiune AUthority
	//ARCOUNT - specifica numarul de RR din sectiunea Additional
	
	header.ancount = ntohs(header.ancount);
	header.nscount = ntohs(header.nscount);
	header.arcount = ntohs(header.arcount);
 
		
	//Avem si un pointer global, care sa pointeze la inceputul mesajului	
	buffer = buf ;
	
	//Raspunsul propriuzis incepe dupa antet si intrebare , iar noi stime
	//dimensiune deoarece noi am trimis aceasta prima parte catre serverul DNS
	poz = initial_len;
	
	//Parsam pe rand datele pentru fiecare sectiune si le scriem in fisierul dns.log
	fprintf(fm, ";; ANSWER SECTION:\n");
	afiseazaSectiune(header.ancount);
	fprintf(fm, "\n");
	
	fprintf(fm, ";; AUTHORITY SECTION:\n");
	afiseazaSectiune(header.nscount);
	fprintf(fm, "\n");
	
	fprintf(fm, ";; ADDITIONAL SECTION:\n");
	afiseazaSectiune(header.arcount);
	
	fclose(fm);
	
}