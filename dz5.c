#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SEPARATOR ','
#define BR_POLJA 30
#define VELICINA_POLJA 25
#define ALOK(pokazivac,n,tip,lista) {\
									pokazivac=realloc(pokazivac,(n)*sizeof(tip));\
									if(!pokazivac){\
										printf("MEM_GRESKA");\
										oslobodi(lista);\
										exit(0);\
									}\
}
#define ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta,trenutno_upisano,pokazivac,n,tip,lista) {\
if (aloc_mesta <= trenutno_upisano + 1) { ALOK(pokazivac, n, tip, lista) aloc_mesta += 10; }\
}


typedef struct elem {
	struct elem *prev;
	char **data;
	struct elem *next;
}Elem;

typedef struct list {
	Elem *prvi;
	Elem *poslednji;
}Lista;

void oslobodi(Lista lista)
{
	Elem *tek = lista.prvi, *stari;
	while (tek)
	{
		stari = tek;
		tek = tek->next;	
		for (int i = 0; stari->data[i] != NULL; i++)	free(stari->data[i]);
		free(stari->data);
		free(stari);
	}
}

char safe_fgetc(FILE *file, Lista lista) {
	char c = fgetc(file);
	if (c == EOF) {
		printf("DAT_GRESKA");
		oslobodi(lista);
		exit(0);
	}
	else return c;
}

//cita jedno polje iz fajla, definisano .csv formatom
//ako je polje ogradjeno znacima navoda, prepisuje se takvo kakvo je
//vraca string ciji je prvi element '\0' ukoliko je polje prazno; vraca NULL ukoliko je polje samo \n ili EOF (sto oznacava kraj zapisa)
char *readField(FILE *file, Lista lista)	
{
	char *field = NULL;
	ALOK(field, VELICINA_POLJA, char, lista)
	int aloc_mesta = VELICINA_POLJA;
	char c = fgetc(file); //ne moze safe verzija, jer prvi procitan karakter moze biti EOF, sto ukazuje na kraj fajla(ne na gresku pri rukovanju sa fajlom)
	int i = 0;
	switch (c)
	{
	case '\n':
	case EOF:
		free(field);
		return NULL;	//ovakvo polje oznacava kraj zapisa
	case '"':			//polje ogradjeno znacima navoda
		field[i++] = c;
		while (c=safe_fgetc(file,lista))
		{
			ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta,i,field,aloc_mesta+10,char,lista)
			field[i++] = c;

			if (c == '"')									//ukoliko je pronadjen znak '"', nakon njega moze se nalaziti ili jos jedan znak '"', ili ','
				if ((c = safe_fgetc(file,lista)) == '"')	//ako je sledeci znak '"', upisati ga i nastaviti sa citanjem polja				
				{
					ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta,i,field, aloc_mesta + 10, char, lista)
					field[i++] = c;
				}
				else
				{
					if (c == SEPARATOR) break;			//ako se ',' nalazi nakon jednog znaka '"', doslo se do kraja polja
					else {
						printf("GRESKA"); break;	//"xxx"x nije validno polje 
					}
				}
		}
		break;
	default:	//obicno polje, neogradjeno anvodnicima
		while (c != SEPARATOR)
		{
			ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta,i,field, aloc_mesta + 10, char, lista)
			field[i++] = c;
			c = safe_fgetc(file,lista);
		}
	}
	field[i++] = '\0';
	ALOK(field, i, char, lista)
	return field;
}

//cita jedan zapis iz fajla
//nakon svih validnih stringova(polja) u zapisu, nalazi se NULL pokazivac da oznaci kraj zapisa
//fja vraca NULL ako nema zapisa
char **readRecord(FILE *file, Lista lista)	
{
	char **rekord = NULL;
	ALOK(rekord, BR_POLJA, char*, lista)
	int aloc_mesta = BR_POLJA;
	int i = 0;
	while ((rekord[i] = readField(file, lista)) != NULL) {	//upisace se NULL pokazivac kada se stigne do kraja zapisa
		ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta,i,rekord,aloc_mesta+10,char*,lista)
		i++;
	}
	if (i == 0) {
		free(rekord);
		return NULL;
	}
	ALOK(rekord, i+1, char*, lista)
	return rekord;
}

//formira jedan element i vraca vrednost pokazivaca na taj element
Elem *formiraj_element(char **rekord, Lista lista)
{
	Elem *element = NULL;
	if (rekord == NULL) return NULL;	//Ako readRecord vrati NULL, to oznacava kraj datoteke(nema vise zapisa)
	ALOK(element, 1, Elem, lista)
	element->data = rekord;
	element->next = element->prev = NULL;
	return element;
}

//cita prosledjeni fajl, formira elemente, povezuje ih u listu
Lista formiranje_liste(FILE *file) 
{
	Lista lista = { NULL,NULL };
	Elem *preth, *novi;
	lista.prvi = formiraj_element(readRecord(file, lista), lista);
	if (lista.prvi)	preth = lista.prvi;
	else return lista;	//nema elemenata liste, vraca NULL NULL
	while (novi = formiraj_element(readRecord(file, lista), lista))
	{
		preth->next = novi;
		novi->prev = preth;
		preth = novi;
	}
	lista.poslednji = preth;
	return lista;
}


//cita jednu adresu, koja pocinje tacno od karaktera na koij pokazuje prosledjeni parametar, alocira memoriju u koju smesta adresu, i vraca pokazivac na nju
//dok cita polje, menja vrednost prosledjenog parametra tako da, kada zavrsi sa radom, pokazuje na pocetak sledece email adrese u polju, ili '\0' ukoliko je procitano celo polje
char *readEmail(char **trenutni, Lista lista) {
	if (**trenutni == '\0')	return NULL;
	char *mail = NULL;
	ALOK(mail, VELICINA_POLJA, char, lista)
	int aloc_mesta = VELICINA_POLJA;
	int j = 0;
	for (;**trenutni!='\0'; (*trenutni)++)
	{
		switch (**trenutni)
		{
		case ' ':
		case '"': break;
		case SEPARATOR:
			mail[j++] = '\0';
			ALOK(mail, j, char, lista)
			(*trenutni)++;
			return mail;
		default:
			ALOKACIJA_DODATNE_MEMORIJE(aloc_mesta, j, mail, aloc_mesta + 10, char, lista)
			mail[j++] = **trenutni;
		}
	}
	mail[j++] = '\0';
	ALOK(mail, j, char, lista)
	return mail;
}

int pronadjiIndeksOd(char *ime_trazenog_polja,Lista lista)
{

	for (int indeks = 0; lista.prvi->data[indeks]; indeks++)
	{
		if (!strcmp(ime_trazenog_polja, lista.prvi->data[indeks])) return indeks;
	}
	return -1;	//ako trazeno polje ne postoji
}

//pravi novi element za svaku adresu iz polja Secondary Email, i dodaje ih u listu 
//vraca pokazivac na posleddnji element koji je dodat u listu
Elem *split_one(Elem *element, Lista *lista)	
{
	int indeksPolja_SecondaryEmail = pronadjiIndeksOd("Secondary Email", *lista);
	int indeksPolja_PrimaryEmail = pronadjiIndeksOd("Primary Email", *lista);
	/*for (int i = 0; lista->prvi->data[i]; i++)
	{
		if (!strcmp("Secondary Email", lista->prvi->data[i])) indeksPolja_SecondaryEmail = i;
		else if (!strcmp("Primary Email", lista->prvi->data[i])) indeksPolja_PrimaryEmail = i;
	}*/
	char *emailAddress = NULL;
	char *trenutniChar = element->data[indeksPolja_SecondaryEmail];
	Elem *prethodni = element, *nastavak = element->next;
	while (emailAddress = readEmail(&trenutniChar, *lista)) {
		char **data_edited = NULL;
		int br_polja = 0;	//broj validnih polja
		for (; lista->prvi->data[br_polja]; br_polja++);
		ALOK(data_edited, br_polja+1, char*, *lista)

		for (int i = 0; element->data[i]; i++) { //kopiranje svih podataka kontakta, osim polja Primary i Secondary Email
			data_edited[i] = NULL;
			if (i == indeksPolja_PrimaryEmail || i == indeksPolja_SecondaryEmail) continue;
			ALOK(data_edited[i], strlen(element->data[i]) + 1, char, *lista)
			strcpy(data_edited[i],element->data[i]);
		}
		data_edited[br_polja] = NULL; //na ovom indeksu treba da se nalazi NULL polje(nakon poslednjeg validnog polja)
		
		data_edited[indeksPolja_PrimaryEmail] = emailAddress; 
		ALOK(data_edited[indeksPolja_SecondaryEmail], 1, char, *lista)
		data_edited[indeksPolja_SecondaryEmail][0] = '\0'; 

		Elem *novi = formiraj_element(data_edited, *lista);
		novi->prev = prethodni;
		prethodni->next = novi;
		prethodni = novi;
	}
	//izvornom elementu(originalnom kontaktu) treba obrisati sadrzaj polja secondary email
	ALOK(element->data[indeksPolja_SecondaryEmail], 1, char, *lista)
	element->data[indeksPolja_SecondaryEmail][0] = '\0';
	prethodni->next = nastavak;
	if (nastavak) nastavak->prev = prethodni;
	else lista->poslednji = prethodni;		//ako ne postoji element nakon onog koji je splitovan, treba izmeniti strukturu lista tako da pokazuje na poslednji element koji je ubacen
	return prethodni;
}


Lista split(Lista lista)
{
	int indeksPolja_SecondaryEmail = pronadjiIndeksOd("Secondary Email",lista);
	Elem *tek = lista.prvi->next;	//preskace se prvi element liste, jer je to zapis formata, ne treba ga splitovati
	while (tek)
	{
		if (tek->data[indeksPolja_SecondaryEmail][0] != '\0') {
			tek = split_one(tek, &lista);
		}
		tek = tek->next;
	}
	return lista;
}

//upis u izlazni fajl
void upis(FILE *file, Lista lista)
{
	Elem *tek = lista.prvi;	
	for (; tek; tek = tek->next)
	{
		int provera = 0;
		for (int i = 0; tek->data[i]; i++) {
			provera=fprintf(file, "%s\t", tek->data[i]);
			if (provera < 0) break;
		}

		if (provera < 0 || fputc('\n', file) == EOF) {
			printf("DAT_GRESKA");
			oslobodi(lista);
			exit(0);
		}
	}
}


int main(int argc, char **argv)
{
	if (argc >= 3)
	{
		FILE *fileInput = fopen(argv[1], "r");
		FILE *fileOutput = fopen(argv[2], "w");

		Lista lista = { NULL,NULL };
		lista = formiranje_liste(fileInput);
		fclose(fileInput);
		if (lista.prvi == NULL) {
			fclose(fileOutput);
			oslobodi(lista);
			return 0;
		}
		if (argc > 3 && !strcmp("-split", argv[3])) lista = split(lista);
		upis(fileOutput, lista);
		fclose(fileOutput);
		oslobodi(lista);
	}
	else printf("ARG_GRESKA");
	return 0;
}

