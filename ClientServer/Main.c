#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "sqlite3/sqlite3.h"


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5000"

int __cdecl main(int argv, char **argc)
{
	sqlite3 *db;
	sqlite3_open("Sales.db", &db);

	if (db == NULL)
	{
		printf("Failed to open DB\n");
		return 1;
	}
	else {
		printf("Database opened\n");
	}


    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char *recvbuf[DEFAULT_BUFLEN] = { '\0' }; 
    int recvbuflen = DEFAULT_BUFLEN;
 
	char *query[DEFAULT_BUFLEN] = { '\0' };

	

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

	

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
		
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();

        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
	}
	else {
		printf("Connected\n");

	}

    // No longer need server socket
    closesocket(ListenSocket);

	char *GetProducts = "GetProducts";



    // Receive until the peer shuts down the connection
    do {
		printf("Listening\n");
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
			printf("Client : %s\n", recvbuf);
			

        // Echo the buffer back to the sender

			if (strcmp(recvbuf, GetProducts) == 0 ) {
				char *sql;
				sqlite3_stmt *stmt;

				sql = "select * from Product";

				printf("Performing query...\n");
				sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
				

				printf("Got results:\n");
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *charr[DEFAULT_BUFLEN] = { '\0' };

					char *ProductName = sqlite3_column_text(stmt, 1);
					int VatRate = sqlite3_column_int(stmt, 2);
					double UnitPrice = sqlite3_column_double(stmt, 3);


					strcat_s(query, sizeof query, ProductName);
					strcat_s(query, sizeof query, "\n");
					sprintf_s(charr, sizeof charr, "%ld", VatRate);
					strcat_s(query, sizeof query, charr);
					strcat_s(query, sizeof query, "\n");
					sprintf_s(charr, sizeof charr, "%lg", UnitPrice);
					strcat_s(query, sizeof query, charr);
					strcat_s(query, sizeof query, "\n");
				}
				strcat_s(query, sizeof query, "END\n");

				iSendResult = send(ClientSocket, query, strlen(query), 0);
				memset(query, 0, sizeof query);
			
			}
			else if (strcmp(recvbuf, "Sales") == 0) {
				memset(recvbuf, 0, sizeof recvbuf);
				recv(ClientSocket, recvbuf, recvbuflen, 0);


				char *sql = "INSERT INTO Sales(ReceiptCount,TotalAmount,CashPayment,CreditPayment) VALUES";
				char *query[DEFAULT_BUFLEN] = { '\0' };

				strcat_s(query, sizeof query, sql);
				strcat_s(query, sizeof query, recvbuf);

				char *zErrMsg = 0;
				int rc;

				char *next = NULL;
				char *first = strtok_s(query, "-", &next);


				printf("Performing query...\n");
				printf("Query :%s\n", first);

				rc = sqlite3_exec(db, first, NULL, NULL, &zErrMsg);

				if (rc != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				else {
					fprintf(stdout, "Operation done successfully\n");
				}
				first = strtok_s(NULL, "-", &next);
				do
				{
					char *part;
					char *posn;

					part = strtok_s(first, " ", &posn);
					char *id = part;
					part = strtok_s(NULL, " ", &posn);
					char *count = part;
					char *query2[DEFAULT_BUFLEN] = { '\0' };


					
					char *update = "UPDATE SaleDetails SET Amount = Amount +";
					char *update2 = "*(SELECT P.UnitPrice FROM Product P WHERE P.Id =";
					char *update3 = ")WHERE ProductId = ";
						
					strcat_s(query2, sizeof query2, update);
					strcat_s(query2, sizeof query2, count);
					strcat_s(query2, sizeof query2, update2);
					strcat_s(query2, sizeof query2, id);
					strcat_s(query2, sizeof query2, update3);
					strcat_s(query2, sizeof query2, id);
					strcat_s(query2, sizeof query2, ";");

					printf("Query :%s\n", query2);

					rc = sqlite3_exec(db, query2, NULL, NULL, &zErrMsg);

					if (rc != SQLITE_OK) {
						fprintf(stderr, "SQL error: %s\n", zErrMsg);
						sqlite3_free(zErrMsg);
					}
					else {
						fprintf(stdout, "Operation done successfully\n");	
					
					}
					if (sqlite3_changes(db) == 0) {

						char *sql = "INSERT INTO SaleDetails(ProductId,ProductName,Amount) VALUES(";
						char *sql2 = ",(SELECT ProductName FROM Product Where Id =";
						char *sql3 = "),(";
						char *sql4 = "*(SELECT UnitPrice FROM Product Where Id =";
						char *sql5 = ")));";
		
						char *query[DEFAULT_BUFLEN] = { '\0' };
						strcat_s(query, sizeof query, sql);
						strcat_s(query, sizeof query, id);
						strcat_s(query, sizeof query, sql2);
						strcat_s(query, sizeof query, id);
						strcat_s(query, sizeof query, sql3);
						strcat_s(query, sizeof query, count);
						strcat_s(query, sizeof query, sql4);
						strcat_s(query, sizeof query, id);
						strcat_s(query, sizeof query, sql5);
						printf("Query :%s\n", query);

						rc = sqlite3_exec(db, query, NULL, NULL, &zErrMsg);

						if (rc != SQLITE_OK) {
							fprintf(stderr, "SQL error: %s\n", zErrMsg);
							sqlite3_free(zErrMsg);
						}
						else {
							fprintf(stdout, "Operation done successfully\n");

						}
					}


				} while ((first = strtok_s(NULL, "-", &next)) != NULL);
				



				memset(recvbuf, 0, sizeof recvbuf);

			}
			
			memset(recvbuf, 0, sizeof recvbuf);

            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
		
                return 1;
            }
			printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
		
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();

        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}