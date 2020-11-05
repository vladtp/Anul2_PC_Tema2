##################################################################################################
TIRIPLICA VLAD-PETRE 321CA
TEMA2 PC
##################################################################################################
Functionare server:
- se deschid soketii pentru TCP si UDP si se adauga in fd_set alaturi de stdin
- se asteapta ca unul din descriptori sa fie selectati

- daca a fost selectat stdin se verifica daca comanda este exit si se inchide serverul alaturi de
clienti

- daca daca a fost selectat soketul de TCP se fac urmatoarele:
- se accepta conectarea unui nou client TCP adaugandu-se noul soket creat la fs_set
- se primeste un mesaj de la client care contine ID-ul sau
- se adauga id ul si socketul in 2 hashtable-uri care mapeaza reciproc id <=> socketi activi
- pentru cazul in care un client s-a reconectat se verifica daca acesta are mesaje pe care le-a
cat timp era deconectat si pe care doreste sa le primeasca, in acest caz ii se trimit toate
mesajele in ordinea in care au venit
- se afiseaza un mesaj de forma CLIENT_ID connected

- daca se primeste un mesaj de la un client TCP se fac urmatoarele:
- se verifica daca acesta a inchis conexiunea, caz in care se elibereaza socketul si se updateaza
structurile de date ce contin informatii legate de acest client
- altfel se verifica daca este un mesaj de tipul subscribe sau unsubscribe conform unui protocol
descris mai joc
- in ambele cazuri se updateaza un hastable care mapeaza topicul la o structura de date ce contine
id-ul clientilor abonati la topicul respectiv dar si daca acestia doresc store&forward

- daca se primeste mesaj de la un client UDP se fac urmatoarele:
- se creeaza mesajul care va fi trimis catre clientii TCP conform unui alt protocol descris mai jos
- se verifica folosind hastable-ul topic_to_id care clienti sunt abonati la acest topic si care au
optiunea de S&F
- se trimite la fiecare client activ mesajuk
- in cazul in care clientii nu sunt activi se verifica daca acestia au optiunea de S&F, caz in care
mesajul este retint intr-un alt hastable a carui cheie sunt id-urile clientilor, iar valorile sunt o
lista de mesaje gata sa fie trimise

Functionare subscriber:
- clientul creaza un soket TCP si se conecteaza la server folosind din nou un fd_set pentru a 
multiplexa intre a primi date de la server sau de la tastatura
- daca s-a primit o comanda de la tastatura aceasta este verificata daca este corecta si se creeaza
un mesaj care va fi trimis catre server specificand datele necesare pentru a realiza comanda
- daca s-a primit un mesaj de la server acesta este afisat la stdout in formatul cerut

PROTOCOALE UTILIZATE:
- mesajele de la client la server folosesc urmatoare structura de date cli_msg definita in helpers.h
- acestea trimit un cod al operatiei (subscribe, unsibscribe, send_id) un mesaj text care poate fi
topicul sau id-ul clientului si optiunea de store and forward
- mesajele sunt de forma [op_code | topic | s&f]
- la crearea unei conexiuni cu un server clientul trimite imediat dupa un mesaj cu id-ul sau si cu
operatia corespunzatoare

- mesajele de ca server la clinet folosesc structura srv_msg din helpers.h 
- acestea trimit topicul, tipul de date, payload-ul dar si ip-ul si portul clientului UDP
- mesajele arata de forma [topic | data_type | payload | ip_src | port_src]
- serverul se ocupa doar de trimiterea lor, iar de extragere informatiilor precum a tipului de
date si a valorilor se ocupa clietul

Obesrvatii:
- am decis ca liniile codului sa nu depaseasca 100 de caractere (considerand ca 80 este prea mic)
- programul a fost scris si testat pe un sistem de operare MACOS X pe care ruleaza asa cum trebuie
si la viteze mari
- cand l-am testat pe masina virtuala de linux am obesrvat ca acesta avea ceva probleme cand datele 
se transferau la viteza mare, dar nu am reusit sa-mi dau seama de ce (este posibil sa fie datorita
limitarii resurselor pe care masina virtuala le impune)
