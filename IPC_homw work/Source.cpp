#include <fstream>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <vector>
#include <string>
#include <iostream>
#include "D:/Crypto++/cryptopp820/hex.h"
#include "D:/Crypto++/cryptopp820/cryptlib.h"
#include "D:/Crypto++/cryptopp820/md5.h"
#include "D:/Crypto++/cryptopp820/base64.h"
#include "D:/Crypto++/cryptopp820/eccrypto.h"
#include "D:/Crypto++/cryptopp820/rsa.h"
#include "D:/Crypto++/cryptopp820/filters.h"

const int MAX_BUFF_SIZE = 3072; //

int main(int argc, char *argv[])
{
	char choice = 'm';
	std::cout << "Choose your role: 's' for server, 'c' for client: ";
	std::cin >> choice;
	switch (choice)
	{
	case 's':
	{
		//create named pipe
		bool is_connected = false;

		HANDLE h_named_pipe;
		LPSTR pipe_name = LPSTR("\\\\.\\pipe\\$firstPipe$");
		std::vector<char> buffer;
		DWORD received_bytes = 0, total_received_bytes = 0, writen_byte = 0, total_writen_bytes = 0;
		h_named_pipe = CreateNamedPipe(pipe_name,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			MAX_BUFF_SIZE, MAX_BUFF_SIZE, 5000, NULL);

		if (h_named_pipe == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Pipe wasn't create. Error # " << GetLastError() << std::endl;
			break;
		}

		//create connection for named pipe
		std::cout << "Waiting for connection..." << std::endl;
		int result = ConnectNamedPipe(h_named_pipe, NULL);

		if (!result)
		{
			std::cerr << "Connection wasn't create. Error #  " << GetLastError() << std::endl;
			CloseHandle(h_named_pipe);
			break;
		}
		std::cout << "Connection created " << std::endl;

		//get file size
		int file_size = 0;
		if (!ReadFile(h_named_pipe, &file_size, sizeof(int), &received_bytes, NULL))
		{
			std::cerr << "Error receiving data " << std::endl;
			CloseHandle(h_named_pipe);
			break;
		}
		std::cout << "file size = " << file_size << std::endl;
		received_bytes = 0;
		buffer.resize(file_size);
		while (total_received_bytes != file_size)
		{
			if (!ReadFile(h_named_pipe, &buffer[0] + total_received_bytes, file_size - total_received_bytes, &received_bytes, NULL))
			{
				std::cerr << "Error receiving data " << GetLastError() << std::endl;
				CloseHandle(h_named_pipe);
				break;
			}
			total_received_bytes += received_bytes;
		}
		if (total_received_bytes == file_size)
		{
			std::cout << "File received successful" << std::endl;
		}

		//looking his hash
		CryptoPP::MD5 hash;
		std::string str_for_hash(buffer.begin(), buffer.end());
		hash.Update((const byte*)str_for_hash.data(), str_for_hash.size());
		std::string result_str;
		result_str.resize(hash.DigestSize() / 2);
		hash.TruncatedFinal((byte*)&result_str[0], result_str.size());
		std::cout << "File hash: \n" << result_str;

		//close connection
		DisconnectNamedPipe(h_named_pipe);
		CloseHandle(h_named_pipe);
	}
	break;
	case 'c':
	{
			if(argc < 1)
			{
				std::cerr << "Enter file name in command line" << std::endl;
				break;
			}
			std::string default_file_name(argv[1]);

		HANDLE h_named_pipe;
		DWORD received_bytes = 0, total_received_bytes = 0, writen_byte = 0, total_writen_bytes = 0;
		std::vector <char> buffer, file_name(MAX_BUFF_SIZE);
		LPSTR pipe_name = LPSTR("\\\\.\\pipe\\$firstPipe$");

		h_named_pipe = CreateFile(pipe_name,
			GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);

		if (h_named_pipe == INVALID_HANDLE_VALUE)
		{
			std::cout << "Error creating pipe. Error # " << GetLastError() << std::endl;
			break;
		}

		std::ifstream in;
		in.open(default_file_name, std::ios_base::binary);
		if (!in.is_open())
		{
			std::cerr << "Error open file. Error # " << GetLastError() << std::endl;
			CloseHandle(h_named_pipe);
			break;
		}

		in.seekg(0, std::ios::end);
		long file_size = in.tellg();

		//send file size and file to server
		if (!WriteFile(h_named_pipe, &file_size, sizeof(int), &writen_byte, NULL))
		{
			std::cerr << "Error receiving file size. Error # " << GetLastError() << std::endl;
			CloseHandle(h_named_pipe);
			break;
		}

		in.seekg(0, std::ios::beg);
		std::string str_buff, tmp_buff;
		//copy file into vector <char> or string
		while (in)
		{
			std::getline(in, tmp_buff);
			str_buff += tmp_buff;
			if (in.fail())
			{
				break;
			}
		}

		std::copy(str_buff.begin(), str_buff.end(), std::back_inserter(buffer));
		writen_byte = 0;
		total_writen_bytes = 0;
		while (total_writen_bytes != file_size)
		{
			if (!WriteFile(h_named_pipe, &buffer[0] + total_writen_bytes, file_size - total_writen_bytes, &writen_byte, NULL))
			{
				CloseHandle(h_named_pipe);
				break;
			}
			total_writen_bytes += writen_byte;
		}

		std::cout << "File send successful" << std::endl;
		in.close();
		CloseHandle(h_named_pipe);

	}
	break;
	}

	system("pause");
	return 0;
}