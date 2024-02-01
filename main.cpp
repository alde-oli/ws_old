#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <map>
#include <ctime>
#include <signal.h>
#include <fcntl.h>


#define PORT 4040


#include "htmlrequest.hpp"

// Prototypes des fonctions

int setupServerSocket();
int handleNewConnection(int server_fd, int kq, std::map<int, time_t>& clientActivity);
bool handleClientRequest(int client_fd, std::map<int, time_t>& clientActivity, std::map<int, std::string>& clientDataToSend);
void checkClientTimeouts(std::map<int, time_t>& clientActivity, int timeout);
int registerWriteEvent(int kq, int fd);
int handleClientWrite(int fd, std::map<int, std::string>& clientActivity);
int unregisterWriteEvent(int kq, int fd);

int main()
{
	int server_fd = setupServerSocket();
	if (server_fd < 0)
		return -1;
	std::map<int, std::string> dataToSend;

	signal(SIGPIPE, SIG_IGN);

	struct timeval sendTimeout;
	sendTimeout.tv_sec = 300;  // Timeout en secondes
	sendTimeout.tv_usec = 0; // Timeout en microsecondes

	// Définir le timeout d'envoi sur le socket
	int val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
	if (setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendTimeout, sizeof(sendTimeout)) < 0)
	{
		perror("setsockopt");
		return -1;
	}

	// Créer une kqueue
	int kq = kqueue();
	if (kq == -1)
	{
		perror("kqueue");
		return -1;
	}

	struct kevent kev;
	EV_SET(&kev, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1) {
		perror("kevent add server_fd");
		close(server_fd);
		return -1;
	}

	struct timespec timeout;
	timeout.tv_sec = 30;  // Réduire pour une vérification plus fréquente
	timeout.tv_nsec = 0;

	std::map<int, time_t> clientActivity;
	const int idle_timeout = 10;  // Timeout d'inactivité en secondes

	while (true)
	{
		struct kevent events[32];
		int nev = kevent(kq, NULL, 0, events, 32, &timeout); // Un seul appel à kevent
		if (nev < 0)
		{
			perror("kevent wait");
			return -1;
		}

		for (int i = 0; i < nev; i++)
		{
			int fd = events[i].ident;

			if (fd == server_fd && events[i].filter == EVFILT_READ) {
				// Gérer une nouvelle connexion
				std::cout << "New connection, socket: " << fd << std::endl;
				int client_fd = handleNewConnection(server_fd, kq, clientActivity);
				if (client_fd != -1) {
					struct kevent client_event;
					EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
					// Note: Pas besoin d'appeler kevent ici, les changements seront appliqués dans le prochain passage de la boucle
				}
			} else if (events[i].filter == EVFILT_READ) {
				// Gérer la demande du client
				std::cout << "Client request, socket: " << fd << std::endl;
				bool hasDataToSend = handleClientRequest(fd, clientActivity, dataToSend);
				if (hasDataToSend) {
					// Enregistrez pour les événements d'écriture si nécessaire
					registerWriteEvent(kq, fd); // Préparer l'événement, appliqué dans le prochain passage
				}
			} else if (events[i].filter == EVFILT_WRITE) {
				// Gérer l'envoi de données au client
				std::cout << "Client write, socket: " << fd << std::endl;
				if (handleClientWrite(fd, dataToSend) == -1)
				{
					// Désinscrire l'événement d'écriture après l'envoi
					std::cout << "Unregister write event, socket: " << fd << std::endl;
					unregisterWriteEvent(kq, fd); // Préparer la désinscription, appliquée dans le prochain passage
				}
			}
		
		}

		checkClientTimeouts(clientActivity, idle_timeout);
	}

	// Fermer le socket serveur
	close(server_fd);

	return 0;
}

int setupServerSocket()
{
	int server_fd;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// Ignorer SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	// Créer le socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		return -1;
	}

	// Rendre le socket non bloquant
	int flags = fcntl(server_fd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl F_GETFL");
		close(server_fd);
		return -1;
	}
	flags |= O_NONBLOCK;
	if (fcntl(server_fd, F_SETFL, flags) == -1)
	{
		perror("fcntl F_SETFL O_NONBLOCK");
		close(server_fd);
		return -1;
	}

	// Définir l'adresse du serveur
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("10.11.14.5");	

	address.sin_port = htons(PORT);

	// Lier le socket
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		close(server_fd);
		return -1;
	}

	// Mettre le serveur en mode écoute
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		close(server_fd);
		return -1;
	}

	std::cout << "Server listening on port " << PORT << std::endl;

	return server_fd;
}

int registerWriteEvent(int kq, int fd) {
	struct kevent kev;
	EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);

	if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1)
	{
		perror("registerWriteEvent: kevent");
		return -1;
	}

	return 0;
}

int handleClientWrite(int fd, std::map<int, std::string>& clientActivity)
{
	// Vérifier si le client a des données à envoyer
	if (clientActivity.find(fd) == clientActivity.end() || clientActivity[fd].empty())
	{
		return -1; // Aucune donnée à envoyer ou client non reconnu
	}

	// Récupérer les données à envoyer
	std::string dataToSend = clientActivity[fd];

	// Envoyer les données
	ssize_t bytesSent = send(fd, dataToSend.c_str(), dataToSend.size(), 0);

	// Gérer les erreurs d'envoi
	if (bytesSent < 0)
	{
		perror("send");
		return -1;
	}

	// Mettre à jour les données restantes à envoyer
	if (bytesSent < static_cast<ssize_t>(dataToSend.size()))
	{
		// Toutes les données n'ont pas été envoyées
		clientActivity[fd] = dataToSend.substr(bytesSent); // Conserver les données non envoyées
		return 0; // Il reste des données à envoyer
	}
	else
	{
		// Toutes les données ont été envoyées
		clientActivity.erase(fd); // Supprimer les données envoyées de la map
		return -1; // Aucune donnée restante à envoyer
	}
}

int unregisterWriteEvent(int kq, int fd)
{
	struct kevent kev;
	EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

	if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1)
	{
		perror("unregisterWriteEvent: kevent");
		return -1;
	}

	return 0;
}

int handleNewConnection(int server_fd, int kq, std::map<int, time_t>& clientActivity)
{
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	struct kevent change_event;

	int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
	if (new_socket < 0)
	{
		perror("accept");
		return -1;
	}

	std::cout << "New client connected, socket: " << new_socket << std::endl;

	EV_SET(&change_event, new_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
	{
		perror("kevent add new socket");
		close(new_socket);
	}
	else
	{
		clientActivity[new_socket] = time(nullptr);  // Enregistrer l'heure de la dernière activité
	}
	return new_socket;
}

bool handleClientRequest(int client_fd, std::map<int, time_t>& clientActivity, std::map<int, std::string>& clientDataToSend)
{
	char 		buffer[1024] = {0};

	std::cout << "Client request, socket: " << client_fd << std::endl;

	ssize_t valread = read(client_fd, buffer, 1024);

	static int	to_read = 0;
	static bool	is_post_body  = 0;
	bool		hasDataToSend = false;

	if (valread > 0)
	{
		std::string buf2(buffer, valread);
		
		static HttpRequest	request;
		if (!is_post_body)
		{
			request = HttpRequest(buf2, client_fd);
			if (request.method == "POST")
			{
				is_post_body = 1;
				to_read = request.headers["Content-Length"].empty() ? 0 : std::atoi(request.headers["Content-Length"].c_str());
				to_read -= request.rawBody.size();
				if (to_read <= 0)
				{
					is_post_body = 0;
					to_read = 0;
				}
			}
		}
		else
		{
			request.rawBody += buf2;
			to_read -= valread;
			if (to_read <= 0)
			{
				is_post_body = 0;
				to_read = 0;
			}
		}

		std::ofstream outfile("test.txt");
		if (1)
		{
			outfile << std::endl << "Client request in class : " << std::endl;
			outfile << "method : " << request.method << std::endl;
			outfile << "uri : " << request.uri << std::endl;
			outfile << "httpVersion : " << request.httpVersion << std::endl;
			std::map<std::string, std::string>::iterator it;
			for (it = request.headers.begin(); it != request.headers.end(); it++)
			{
				outfile << it->first << " <-> " << it->second << std::endl;
			}
			outfile << "body : " << request.rawBody << std::endl;
		}
		outfile.close();

		if (!is_post_body)
			hasDataToSend = request.HandleRequest(clientDataToSend, client_fd);
		clientActivity[client_fd] = time(nullptr);  // Mettre à jour l'heure de la dernière activité

	}
	else if (valread == 0)
	{
		std::cout << "Client disconnected, socket: " << client_fd << std::endl;
		close(client_fd);
		clientActivity.erase(client_fd);  // Supprimer le client de la map
	}
	else
	{
		perror("read");
		close(client_fd);
		clientActivity.erase(client_fd);  // Supprimer le client de la map
	}
	return hasDataToSend;
}

void checkClientTimeouts(std::map<int, time_t>& clientActivity, int timeout)
{
	time_t now = time(nullptr);
	for (std::__1::map<int, time_t>::iterator it = clientActivity.begin(); it != clientActivity.end(); )
	{
		if (now - it->second > timeout)
		{
			std::cout << "Client timed out, socket: " << it->first << std::endl;
			close(it->first);
			it = clientActivity.erase(it);  // Supprimer le client et avancer l'itérateur
		}
		else
		{
			++it;
		}
	}
}
