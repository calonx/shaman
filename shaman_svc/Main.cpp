
#include <windows.h>
#include <iterator>
#include <stdio.h>

int main(int argc, char** argv)
{
	char name[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD name_len = std::size(name);
	GetComputerName(name, &name_len);
	printf("Computer name: %s\n", name);
	return 0;
}